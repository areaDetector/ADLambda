/**
 Copyright (c) 2015, UChicago Argonne, LLC
 See LICENSE file.
 */
/* ADLambda.cpp */
#include <algorithm>
#include <iocsh.h>
#include <vector>
#include <math.h>
#include <string>
#include <cstring>
#include <iostream>

#include <epicsTime.h>
#include <epicsMutex.h>
#include <epicsExit.h>
#include <epicsExport.h>

#include "ADLambda.h"
#include <libxsp.h>


static void acquire_thread_callback(void *drvPvt)    { ((ADLambda*) drvPvt)->waitAcquireThread(); }
static void export_thread_callback(void *drvPvt)     { ((ADLambda*) drvPvt)->exportThread(); }

static void receiver_acquire_callback(void *drvPvt)
{
	acquire_data* data = (acquire_data*) drvPvt;
	
	data->driver->acquireThread(data->receiver);
	
	delete data;
}

void ADLambda::spawnAcquireThread(int receiver)
{
	acquire_data* data = new acquire_data;

	data->driver = this;
	data->receiver = receiver;

	epicsThreadCreate("ADLambda::acquireThread()",
              epicsThreadPriorityMedium,
              epicsThreadGetStackSize(epicsThreadStackMedium),
              (EPICSTHREADFUNC)::receiver_acquire_callback,
              data);
}

extern "C" 
{
	/** Configuration command for Lambda driver; creates a new ADLambda object.
	 * \param[in] portName The name of the asyn port driver to be created.
	 * \param[in] configPath path to the config files.
	 * \param[in] numModules Number of image module in the camera
	 */
	void LambdaConfig(const char *portName, const char* configPath, int numModules) 
	{
		new ADLambda(portName, configPath, numModules);
	}

}

const char *ADLambda::driverName = "Lambda";

/**
 * Constructor
 * \param[in] portName The name of the asyn port driver to be created.
 * \param[in] configPath directory containing configuration file location
 * \param[in] numModules The number of individual camera modules the system contains
 *
 */
ADLambda::ADLambda(const char *portName, const char *configPath, int numModules) :
	ADDriver(portName, 
	         numModules,
			 0,
	         0, 
	         0, 
	         asynEnumMask, 
	         asynEnumMask, 
	         ASYN_CANBLOCK, 
	         1, 
	         0, 
	         0),
configFileName(configPath)
{
	this->startAcquireEvent = new epicsEvent();
	this->dequeLock = new epicsMutex();

	this->threadFinishEvents = (epicsEvent**) calloc(numModules, sizeof(epicsEvent*));
	
	for (int index = 0; index < numModules; index += 1)
	{
		this->threadFinishEvents[index] = new epicsEvent();
	}

	createParam( LAMBDA_ConfigFilePathString,    asynParamOctet,   &LAMBDA_ConfigFilePath);
	createParam( LAMBDA_EnergyThresholdString,   asynParamFloat64, &LAMBDA_EnergyThreshold);
	createParam( LAMBDA_DualThresholdString,     asynParamFloat64, &LAMBDA_DualThreshold);
	createParam( LAMBDA_DecodedQueueDepthString, asynParamInt32,   &LAMBDA_DecodedQueueDepth);
	createParam( LAMBDA_OperatingModeString,     asynParamInt32,   &LAMBDA_OperatingMode);
	createParam( LAMBDA_DualModeString,          asynParamInt32,   &LAMBDA_DualMode);
	createParam( LAMBDA_ChargeSummingString,     asynParamInt32,   &LAMBDA_ChargeSumming);
	createParam( LAMBDA_DetectorStateString,     asynParamInt32,   &LAMBDA_DetectorState);
	createParam( LAMBDA_BadFrameCounterString,   asynParamInt32,   &LAMBDA_BadFrameCounter);
	createParam( LAMBDA_BadImageString,          asynParamInt32,   &LAMBDA_BadImage);
	createParam( LAMBDA_ReadoutThreadsString,    asynParamInt32,   &LAMBDA_ReadoutThreads);
	createParam( LAMBDA_StitchWidthString,       asynParamInt32,   &LAMBDA_StitchedWidth);
	createParam( LAMBDA_StitchHeightString,      asynParamInt32,   &LAMBDA_StitchedHeight);
	
	setIntegerParam(LAMBDA_ReadoutThreads, 0);
	setIntegerParam(LAMBDA_BadFrameCounter, 0);
	
	this->connect();
}

ADLambda::~ADLambda()    { this->disconnect(); }


asynStatus ADLambda::connect()
{
	sys = xsp::createSystem(configFileName);
	
	if (sys == nullptr)
	{
		setStringParam(ADStatusMessage, "Unable to start system\n");
		return asynError;
	}
	
	sys->connect();
	sys->initialize();

	det = std::dynamic_pointer_cast<xsp::lambda::Detector>(sys->detector("lambda"));
	
	det->setEventHandler([&](auto t, const void* d) {
		switch (t) 
		{
			case xsp::EventType::READY:
				break;
				
			case xsp::EventType::START:
				this->setIntegerParam(ADStatus, ADStatusAcquire);
				break;
				
			case xsp::EventType::STOP:
			{
				int stat;
				this->getIntegerParam(ADStatus, &stat);
				
				if (stat == ADStatusAcquire)    { this->setIntegerParam(ADStatus, ADStatusReadout); }
				
				break;
			}
		}
		
		this->callParamCallbacks();
	});
	
	
	std::vector<std::string> IDS = sys->receiverIds();
	
	// Calculate the size of the full stitched image
	int full_width = 0, full_height = 0;
	
	for (size_t index = 0; index < IDS.size(); index += 1)
	{
		std::shared_ptr<xsp::Receiver> rec = sys->receiver(IDS[index]);
		
		xsp::Position pos = rec->position();
		
		if (rec->frameWidth() + pos.x > full_width)    { full_width = rec->frameWidth() + pos.x; }
		if (rec->frameHeight() + pos.y > full_height)  { full_height = rec->frameHeight() + pos.y; }
		
		this->writeDepth(rec->frameDepth());
		
		recs.push_back(rec);
	}
	
	setIntegerParam(LAMBDA_StitchedHeight, full_height);
	setIntegerParam(LAMBDA_StitchedWidth, full_width);
	this->setSizes();

	std::string manufacturer = "X-Spectrum GmbH";
	setStringParam(ADManufacturer, manufacturer);

	std::string model = sys->id();
	setStringParam(ADModel, model);

	std::string fwVers = det->firmwareVersion(1);
	setStringParam(ADFirmwareVersion, fwVers);

	std::string version = xsp::libraryVersion();
	setStringParam(ADSDKVersion, version);
		
	std::vector<double> thresholds = det->thresholds();
	if (thresholds.size() > 0)    { setDoubleParam(LAMBDA_EnergyThreshold, thresholds[0]); }
	if (thresholds.size() > 1)    { setDoubleParam(LAMBDA_DualThreshold,   thresholds[1]); }
		
	callParamCallbacks();
	
	this->connected = true;
	
	epicsThreadCreate("ADLambda::waitAcquireThread()",
	                  epicsThreadPriorityLow,
	                  epicsThreadGetStackSize(epicsThreadStackMedium),
	                  (EPICSTHREADFUNC)::acquire_thread_callback,
	                  this);
	                  
	epicsThreadCreate("ADLambda::exportThread()",
	                  epicsThreadPriorityMedium,
	                  epicsThreadGetStackSize(epicsThreadStackMedium),
	                  (EPICSTHREADFUNC)::export_thread_callback,
	                  this);

    return asynSuccess;

}

asynStatus ADLambda::disconnect()
{
	this->lock();
		this->connected = false;
	this->unlock();
	
	return asynSuccess;
}

void ADLambda::setSizes()
{
	int full_width, full_height, dual;
	
	getIntegerParam(LAMBDA_StitchedWidth, &full_width);
	getIntegerParam(LAMBDA_StitchedHeight, &full_height);
	getIntegerParam(LAMBDA_DualMode, &dual);
	
	if (dual)    { full_height *= 2; }

	setIntegerParam(ADMinX, 0);
	setIntegerParam(ADMinY, 0);
	setIntegerParam(ADSizeX, full_width);
	setIntegerParam(ADSizeY, full_height);
	setIntegerParam(ADMaxSizeX, full_width);
	setIntegerParam(ADMaxSizeY, full_height);
	setIntegerParam(NDArraySizeX, full_width);
	setIntegerParam(NDArraySizeY, full_height);
	setIntegerParam(NDArraySize, 0);
	callParamCallbacks();
}

void ADLambda::incrementValue(int param)
{
	int val;
	getIntegerParam(param, &val);
	val += 1;
	setIntegerParam(param, val);
}

void ADLambda::decrementValue(int param)
{
	int val;
	getIntegerParam(param, &val);
	val -= 1;
	setIntegerParam(param, val);
}

void ADLambda::sendParameters()
{
	double shuttertime, low_energy, high_energy;
	int trigger, operation, dual, charge, frames;
	
	getDoubleParam(ADAcquireTime, &shuttertime);
	getDoubleParam(LAMBDA_EnergyThreshold, &low_energy);
	getDoubleParam(LAMBDA_DualThreshold, &high_energy);
	
	getIntegerParam(ADTriggerMode, &trigger);
	getIntegerParam(LAMBDA_OperatingMode, &operation);
	getIntegerParam(LAMBDA_DualMode, &dual);
	getIntegerParam(LAMBDA_ChargeSumming, &charge);
	getIntegerParam(ADNumImages, &frames);
	
	
	// Set Default Values
	xsp::lambda::Gating         gate =  xsp::lambda::Gating::OFF;
	xsp::lambda::TrigMode       tm =    xsp::lambda::TrigMode::SOFTWARE;
	xsp::lambda::CounterMode    cm =    xsp::lambda::CounterMode::SINGLE;
	xsp::lambda::BitDepth       depth = xsp::lambda::BitDepth::DEPTH_1;
	xsp::lambda::ChargeSumming  sum =   xsp::lambda::ChargeSumming::OFF;
	
	if (dual)    { cm = xsp::lambda::CounterMode::DUAL; }
	if (charge)  { sum = xsp::lambda::ChargeSumming::ON; }
	
	// Trigger Mode
	if      (trigger == 1)    { tm = xsp::lambda::TrigMode::EXT_SEQUENCE; }
	else if (trigger == 2)    { tm = xsp::lambda::TrigMode::EXT_FRAMES; }
	else if (trigger == 3)    { gate = xsp::lambda::Gating::ON; }
	
	// Operating Mode
	if      (operation == ONE_BIT)         { depth = xsp::lambda::BitDepth::DEPTH_1; }
	else if (operation == SIX_BIT)         { depth = xsp::lambda::BitDepth::DEPTH_6; }
	else if (operation == TWELVE_BIT)      { depth = xsp::lambda::BitDepth::DEPTH_12; }
	else if (operation == TWENTY_FOUR_BIT) { depth = xsp::lambda::BitDepth::DEPTH_24; }
	
	// Set Values
	det->setGatingMode(gate);
	det->setTriggerMode(tm);
	det->setOperationMode(xsp::lambda::OperationMode(depth, sum, cm));
	det->setShutterTime(shuttertime * 1000);
	
	// Set Thresholds
	std::vector<double> thresholds = det->thresholds();
		
	thresholds[0] = low_energy;
	if (dual)    { thresholds[1] = high_energy; }
		
	det->setThresholds(thresholds);
	det->setFrameCount(frames);
	
	this->callParamCallbacks();
}

/**
 * Background thread to wait until an acquire signal is recieved.
 * Afterwards, start the acquisition and create an acquire thread
 * for each receiver. Then, wait until the threads all finish.
 */
void ADLambda::waitAcquireThread() 
{
	this->lock();
	
	while(this->connected)
	{
		this->unlock();
			bool signal = this->startAcquireEvent->wait(.000025);
		this->lock();
		
		if (!signal)    { continue; }
		
		
		// Sync epics parameters to detector
		try
		{
			this->sendParameters();
			this->setSizes();
			this->setIntegerParam(LAMBDA_BadImage, 0);
			this->callParamCallbacks();
			det->startAcquisition();
			
			this->unlock();
		}
		catch(const xsp::RuntimeError& e)
		{
			
			this->setStringParam(ADStatusMessage, "Failed to start acquisition");
			this->callParamCallbacks();
			continue;
		}
		
		// Spawn acquisition threads
		for (size_t rec_index = 0; rec_index < this->recs.size(); rec_index += 1)
		{
			this->setIntegerParam(rec_index, ADNumImagesCounter, 0);
			this->setIntegerParam(rec_index, LAMBDA_BadFrameCounter, 0);
			this->incrementValue(LAMBDA_ReadoutThreads);
			this->callParamCallbacks(rec_index);
			this->callParamCallbacks();
			
			this->spawnAcquireThread(rec_index);
		}
		
		// Wait for all threads to finish acquiring
		for (size_t index = 0; index < this->recs.size(); index += 1)
		{
			this->threadFinishEvents[index]->wait();
			
			decrementValue(LAMBDA_ReadoutThreads);
			callParamCallbacks();
		}
		
		det->stopAcquisition();
		
		this->lock();
		
		this->frames.clear();

		this->setIntegerParam(ADAcquire, 0);
		this->callParamCallbacks();
		
		while (! export_queue.empty())    { epicsThreadSleep(0.00025); }
		
		this->setIntegerParam(ADStatus, ADStatusIdle);
		this->callParamCallbacks();
	}
	
	this->unlock();
}

void ADLambda::exportThread()
{
	while(this->connected)
	{
		dequeLock->lock();
			bool sleep = export_queue.empty();
		dequeLock->unlock();
	
		if (sleep)
		{
			epicsThreadSleep(0.00025);
			continue;
		}
		
		if (this->pImage)    { this->pImage->release(); }
		
		dequeLock->lock();
			this->pImage = export_queue.front();
			export_queue.pop_front();
		dequeLock->unlock();
		
		NDArrayInfo info;
		this->pImage->getInfo(&info);
		incrementValue(NDArrayCounter);
			
		this->setIntegerParam(NDArraySize, info.totalBytes);
		
		int arrayCallbacks;
		getIntegerParam(NDArrayCallbacks, &arrayCallbacks);
		
		if (arrayCallbacks)    { doCallbacksGenericPointer(this->pImage, NDArrayData, 0); }
	}
}
		
void ADLambda::acquireThread(int receiver)
{
	std::shared_ptr<xsp::Receiver> rec = this->recs[receiver];

	int width, height, toRead, datatype, dual_mode, depth;
	double exposure;
	
	this->getIntegerParam(ADMaxSizeX, &width);
	this->getIntegerParam(ADMaxSizeY, &height);
	this->getIntegerParam(ADNumImages, &toRead);
	this->getIntegerParam(NDDataType, &datatype);
	this->getIntegerParam(LAMBDA_OperatingMode, &depth);
	this->getIntegerParam(LAMBDA_DualMode, &dual_mode);
	this->getDoubleParam(ADAcquireTime, &exposure);

	int bytes_per_pixel = 1;
	
	if      (depth == TWELVE_BIT)       { bytes_per_pixel = 2; }
	else if (depth == TWENTY_FOUR_BIT)  { bytes_per_pixel = 4; }
	
	size_t imagedims[2] = { (size_t) width, (size_t) height};

	xsp::Frame* acquired[2] = { NULL, NULL };
	
	int numAcquired = 0;
	int dual = 0;
	
	const int frame_width = rec->frameWidth();
	const int frame_height = rec->frameHeight();
	
	xsp::Position modulepos = rec->position();
	const int x_shift = (int) modulepos.x;
	const int y_shift = (int) modulepos.y;
	
	NDArrayInfo info;
	
	while (numAcquired < toRead)
	{
		xsp::Frame* temp = (xsp::Frame*) rec->frame(1500);
		
		if (temp == nullptr || temp->data() == NULL)
		{
			if (det->isBusy())    { continue; }
			else                  { break; }
		}
		
		acquired[dual] = temp;
		
		if (dual_mode && !dual)    { dual = 1; continue; }
		else                       { dual = 0; }
		
		// If not in dual mode, will just take the first status twice
		int bad_frame = ((int) acquired[0]->status() | (int) acquired[dual_mode]->status()); 
	
		const int frame_no = acquired[0]->nr();
		
		this->lock();
			if (this->frames.find(frame_no) == this->frames.end())
			{
				epicsTimeStamp currentTime = epicsTime::getCurrent();
				
				NDArray* new_frame = pNDArrayPool->alloc(2, imagedims, (NDDataType_t) datatype, 0, NULL);
				new_frame->uniqueId = 0;
				new_frame->getInfo(&info);
				new_frame->timeStamp = currentTime.secPastEpoch + currentTime.nsec / ONE_BILLION;
				updateTimeStamp(&(new_frame->epicsTS));
			
				memset((char*) new_frame->pData, 0, imagedims[0] * imagedims[1] * info.bytesPerElement);

				this->frames[frame_no] = new_frame;
				
				incrementValue(ADNumImagesCounter);
			}
		this->unlock();
		
		NDArray* output = this->frames[frame_no];
		output->getInfo(&info);
		numAcquired += 1;
		
		if (bad_frame == (int) xsp::FrameStatusCode::FRAME_OK)
		{
			for (int which = 0; which <= dual_mode; which += 1)
			{
				char* out_data = (char*) output->pData;
				char* in_data = (char*) acquired[which]->data();
		
				for (int row = 0; row < frame_height; row += 1)
				{
					for (int col = 0; col < frame_width; col += 1)
					{
						int in_offset = (row * frame_width + col) * bytes_per_pixel;
						int out_offset = ((y_shift + row + (height * which)) * width + col + x_shift) * info.bytesPerElement;
						
						epicsUInt32 temp_in;
						std::memcpy(&temp_in, &in_data[in_offset], bytes_per_pixel);
						
						switch ((NDDataType_t) datatype)
						{
							case NDInt8:
							{
								epicsInt8 temp_out = (epicsInt8) temp_in;
								std::memcpy(&out_data[out_offset], &temp_out, 1);
								break;
							}
							
							case NDUInt8:
							{
								epicsUInt8 temp_out = (epicsUInt8) temp_in;
								std::memcpy(&out_data[out_offset], &temp_out, 1);
								break;
							}
							
							case NDInt16:
							{
								epicsInt16 temp_out = (epicsInt16) temp_in;
								std::memcpy(&out_data[out_offset], &temp_out, 2);
								break;
							} 
							
							case NDUInt16:
							{
								epicsUInt16 temp_out = (epicsUInt16) temp_in;
								std::memcpy(&out_data[out_offset], &temp_out, 2);
								break;
							}
							
							case NDInt32:
							{
								epicsInt32 temp_out = (epicsInt32) temp_in;
								std::memcpy(&out_data[out_offset], &temp_out, 4);
								break;
							} 
							
							case NDUInt32:
							{
								epicsUInt32 temp_out = (epicsUInt32) temp_in;
								std::memcpy(&out_data[out_offset], &temp_out, 4);
								break;
							}
							
							case NDFloat32:
							{
								epicsFloat32 temp_out = (epicsFloat32) temp_in;
								std::memcpy(&out_data[out_offset], &temp_out, 4);
								break;
							}
							
							case NDFloat64:
							{
								epicsFloat64 temp_out = (epicsFloat32) temp_in;
								std::memcpy(&out_data[out_offset], &temp_out, 8);
								break;
							}
						}
					}
				}
			}
		}
		
		rec->release(acquired[0]->nr());
		if (dual_mode)    { rec->release(acquired[1]->nr()); }

		
		this->lock();
			int id = output->uniqueId;
			
			if (id < 0)                { id -= 1; }
			else if (bad_frame > 0)    { id = -1 * std::abs(id) - 1; }
			else                       { id += 1; }
		
			if   (std::abs(id) < this->recs.size())    { output->uniqueId = id; }
			else 
			{
				output->uniqueId = frame_no;
				
				if (id < 0)    { incrementValue(LAMBDA_BadFrameCounter); }	
				else
				{ 
					dequeLock->lock();
					this->export_queue.push_back(output); 
					dequeLock->unlock();
				}
				
				this->frames.erase(frame_no);
			}
		this->unlock();
		
		int numBuffered = rec->framesQueued();
		this->setIntegerParam(receiver, LAMBDA_DecodedQueueDepth, numBuffered);
		callParamCallbacks(receiver);
	}
	
	this->threadFinishEvents[receiver]->trigger();
}

/**
 * Override super class's report method to provide detector specific info.
 * When done, call ADDriver (direct super class) method to provide info from
 * the upper classes.
 */
void ADLambda::report(FILE *fp, int details) 
{
	ADDriver::report(fp, details);
}

asynStatus  ADLambda::readInt32 (asynUser *pasynUser, epicsInt32 *value)
{
	int status = asynSuccess;
	int function = pasynUser->reason;

	if (function == LAMBDA_DetectorState)
	{
		bool det_busy = det->isBusy();
		bool sys_busy = sys->isBusy();

		if (!det_busy && !sys_busy)    { *value = 0; }
		else                           { *value = 2; }
	}
	else 
	{
		if (function < LAMBDA_FIRST_PARAM) 
		{
			status = ADDriver::readInt32(pasynUser, value);
		}
	}
	callParamCallbacks();
	return (asynStatus) status;
}

void ADLambda::writeDepth(int depth)
{
	setIntegerParam(LAMBDA_OperatingMode, depth);

	if      (depth == ONE_BIT)            { setIntegerParam(NDDataType, NDUInt8); }
	else if (depth == SIX_BIT)            { setIntegerParam(NDDataType, NDUInt8); }
	else if (depth == TWELVE_BIT)         { setIntegerParam(NDDataType, NDUInt16); }
	else if (depth == TWENTY_FOUR_BIT)    { setIntegerParam(NDDataType, NDUInt32); }
}


/**
 * Override from super class to handle detector specific parameters.
 * If the parameter is from one of the super classes and is not handled
 * here, then pass along to ADDriver (direct super class)
 */
asynStatus ADLambda::writeInt32(asynUser *pasynUser, epicsInt32 value) 
{
	int status = asynSuccess;

	int addr;
	int function;
	const char* paramName;

	status = parseAsynUser(pasynUser, &function, &addr, &paramName);
	if (status != asynSuccess)    { return (asynStatus) status; }

	//Record for later use
	int adStatus;
	int adacquiring;
	getIntegerParam(ADStatus, &adStatus);
	getIntegerParam(ADAcquire, &adacquiring);

	/** Make sure that we write the value to the param */
	setIntegerParam(addr, function, value);

	if (function == ADAcquire)
	{
		if (value && (adStatus == ADStatusIdle) && (adacquiring == 0))    { this->startAcquireEvent->trigger(); }
		else if (!value && adacquiring)
		{ 
			det->stopAcquisition();
			this->setIntegerParam(ADStatus, ADStatusAborting);
		}
	}
	else if (function == LAMBDA_OperatingMode)
	{
		this->writeDepth(value);
	}
	else if (function < LAMBDA_FIRST_PARAM) 
	{
		status = ADDriver::writeInt32(pasynUser, value);
	}

	callParamCallbacks(addr);
	return (asynStatus) status;
}


/* Code for iocsh registration */

/* LambdaConfig */
static const iocshArg LambdaConfigArg0 = { "Port name", iocshArgString };
static const iocshArg LambdaConfigArg1 = { "Config file path", iocshArgString };
static const iocshArg LambdaConfigArg2 = { "numModules", iocshArgInt };
static const iocshArg * const LambdaConfigArgs[] = { &LambdaConfigArg0, &LambdaConfigArg1, &LambdaConfigArg2};

static void configLambdaCallFunc(const iocshArgBuf *args) {
	LambdaConfig(args[0].sval, args[1].sval, args[2].ival);
}
static const iocshFuncDef configLambda = { "LambdaConfig", 3, LambdaConfigArgs };

static void LambdaRegister(void) 
{
	iocshRegister(&configLambda, configLambdaCallFunc);
}

extern "C" 
{
	epicsExportRegistrar(LambdaRegister);
}
