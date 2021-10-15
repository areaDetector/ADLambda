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

static void errlog_callback(xsp::LogLevel lv, const std::string msg)
{
	printf("%s\n", msg.c_str());
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

	this->threadFinishEvents = calloc(numModules, sizeof(epicsEvent*));
	this->saved_frames = calloc(numModules, sizeof(NDArray*));
	
	for (int index = 0; index < numModules; index += 1)
	{
		this->saved_frames[index] = NULL;
		this->threadFinishEvents[index] = new epicsEvent();
	}

	createParam(LAMBDA_ConfigFilePathString, asynParamOctet, &LAMBDA_ConfigFilePath);
	createParam(LAMBDA_EnergyThresholdString, asynParamFloat64, &LAMBDA_EnergyThreshold);
	createParam(LAMBDA_DualThresholdString, asynParamFloat64, &LAMBDA_DualThreshold);
	createParam(LAMBDA_DecodedQueueDepthString, asynParamInt32, &LAMBDA_DecodedQueueDepth);
	createParam(LAMBDA_OperatingModeString, asynParamInt32, &LAMBDA_OperatingMode);
	createParam(LAMBDA_DetectorStateString, asynParamInt32, &LAMBDA_DetectorState);
	createParam(LAMBDA_BadFrameCounterString, asynParamInt32, &LAMBDA_BadFrameCounter);
	createParam(LAMBDA_BadImageString, asynParamInt32, &LAMBDA_BadImage);
	createParam(LAMBDA_ReadoutThreadsString, asynParamInt32, &LAMBDA_ReadoutThreads);
	createParam(LAMBDA_StitchWidthString, asynParamInt32, &LAMBDA_StitchedWidth);
	createParam(LAMBDA_StitchHeightString, asynParamInt32, &LAMBDA_StitchedHeight);
	
	setIntegerParam(LAMBDA_ReadoutThreads, 0);
	setIntegerParam(LAMBDA_BadFrameCounter, 0);
	
	//xsp::setLogHandler(errlog_callback);
	
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
				if (stat == ADStatusAcquire)
				{
					this->setIntegerParam(ADStatus, ADStatusReadout);
				}
				break;
			}
		}
		
		this->callParamCallbacks();
	});
	
	
	std::vector<std::string> IDS = sys->receiverIds();
	
	// Calculate the size of the full stitched image
	int full_width = 0, full_height = 0;
	
	for (int index = 0; index < IDS.size(); index += 1)
	{
		std::shared_ptr<xsp::Receiver> rec = sys->receiver(IDS[index]);
		
		xsp::Position pos = rec->position();
		
		if (rec->frameWidth() + pos.x > full_width)    { full_width = rec->frameWidth() + pos.x; }
		if (rec->frameHeight() + pos.y > full_height)  { full_height = rec->frameHeight() + pos.y; }
		
		int depth = rec->frameDepth();
		
		if (depth == ONE_BIT)
		{
			setIntegerParam(NDDataType, NDUInt8);
			setIntegerParam(LAMBDA_OperatingMode, ONE_BIT_MODE);
		}
		else if (depth == SIX_BIT)
		{
			setIntegerParam(NDDataType, NDUInt8);
			setIntegerParam(LAMBDA_OperatingMode, SIX_BIT_MODE);
		}
		else if (depth == TWELVE_BIT) 
		{
			setIntegerParam(NDDataType, NDUInt16);
			setIntegerParam(LAMBDA_OperatingMode, TWELVE_BIT_MODE);
		}
		else if (depth == TWENTY_FOUR_BIT) 
		{
			setIntegerParam(NDDataType, NDUInt32);
			setIntegerParam(LAMBDA_OperatingMode, TWENTY_FOUR_BIT_MODE);
		}
		
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
		
	this->getThresholds();
		
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

bool ADLambda::dualMode()
{
	int operating_mode;
	getIntegerParam(LAMBDA_OperatingMode, &operating_mode);
	
	return (operating_mode > TWENTY_FOUR_BIT);
}

void ADLambda::setSizes()
{
	int full_width, full_height;
	getIntegerParam(LAMBDA_StitchedWidth, &full_width);
	getIntegerParam(LAMBDA_StitchedHeight, &full_height);
	
	if (dualMode())    { full_height *= 2; }

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

void ADLambda::syncParameters()
{
	double shuttertime;
	int triggermode;
	int operatingmode;
	
	getIntegerParam(ADTriggerMode, &triggermode);
	getIntegerParam(LAMBDA_OperatingMode, &operatingmode);
	getDoubleParam(ADAcquireTime, &shuttertime);
	
	this->setTriggerMode(triggermode);
	this->setOperatingMode(operatingmode);
	
	det->setShutterTime(shuttertime * 1000);
	
	this->callParamCallbacks();
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
		
		int datatype;
		this->getIntegerParam(NDDataType, &datatype);
	
		this->imageDataType = datatype;
		this->setIntegerParam(LAMBDA_BadImage, 0);
		
		this->syncParameters();
		
		try
		{
			det->startAcquisition();
		}
		catch(const xsp::RuntimeError& e)
		{
			
			this->setStringParam(ADStatusMessage, "Failed to start acquisition");
			this->callParamCallbacks();
			continue;
		}

		this->unlock();
		
		for (int rec_index = 0; rec_index < this->recs.size(); rec_index += 1)
		{
			this->setIntegerParam(rec_index, ADNumImagesCounter, 0);
			this->setIntegerParam(rec_index, LAMBDA_BadFrameCounter, 0);
			this->callParamCallbacks(rec_index);
			this->spawnAcquireThread(rec_index);
		}
		
		setIntegerParam(LAMBDA_ReadoutThreads, this->recs.size());
		
		int threads_running = this->recs.size();
		
		for (int index = 0; index < this->recs.size(); index += 1)
		{
			this->threadFinishEvents[index]->wait();
			
			threads_running -= 1;

			setIntegerParam(LAMBDA_ReadoutThreads, threads_running);
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
		
		if (arrayCallbacks)
		{
			doCallbacksGenericPointer(this->pImage, NDArrayData, 0);
		}
	}
}
		
void ADLambda::acquireThread(int receiver)
{
	std::shared_ptr<xsp::Receiver> rec = this->recs[receiver];

	int width, height, toRead;
	double exposure;
	
	this->getIntegerParam(ADMaxSizeX, &width);
	this->getIntegerParam(ADMaxSizeY, &height);
	this->getIntegerParam(ADNumImages, &toRead);
	this->getDoubleParam(ADAcquireTime, &exposure);

	size_t imagedims[2] = {width, height};

	xsp::Frame* acquired[2] = { NULL, NULL };
	
	int dual_mode = this->dualMode() ? 1 : 0;
	if (dual_mode)    { imagedims[1] = height * 2; }
	
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
		xsp::Frame* temp = rec->frame(1500);
		
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
				NDArray* new_frame = pNDArrayPool->alloc(2, imagedims, this->imageDataType, 0, NULL);
				new_frame->uniqueId = 0;
			
				NDArrayInfo main_info;
				new_frame->getInfo(&main_info);
			
				memset((char*) new_frame->pData, 0, imagedims[0] * imagedims[1] * main_info.bytesPerElement);
			
				epicsTimeStamp currentTime = epicsTime::getCurrent();
				new_frame->timeStamp = currentTime.secPastEpoch + currentTime.nsec / ONE_BILLION;
				updateTimeStamp(&(new_frame->epicsTS));
			
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
					int in_offset = row * frame_width * info.bytesPerElement;

					int out_offset = ((y_shift + row + (height * which)) * width + x_shift) * info.bytesPerElement;
				
					std::memcpy(&out_data[out_offset], &in_data[in_offset], frame_width * info.bytesPerElement);
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

/**
 * Override from super class to handle detector specific parameters.
 * If the parameter is from one of the super classes and is not handled
 * here, then pass along to ADDriver (direct super class)
 */
asynStatus ADLambda::writeFloat64(asynUser *pasynUser, epicsFloat64 value) 
{
	int status = asynSuccess;
	int function = pasynUser->reason;

	//Record for later use
	int adStatus;
	int adacquiring;
	getIntegerParam(ADStatus, &adStatus);
	getIntegerParam(ADAcquire, &adacquiring);

	if ((adStatus != ADStatusIdle) || adacquiring)    
	{ 
		setStringParam(ADStatusMessage, "Failed to set value, currently not Idle");
		this->callParamCallbacks();
		return asynError; 
	}
	else
	{
		setStringParam(ADStatusMessage, "");
	}

	setDoubleParam(function, value);
	
	if (function == ADAcquireTime) 
	{
		double shutterTime = value * 1000.0;
		det->setShutterTime(shutterTime);
	}
	else if (function == ADAcquirePeriod)
	{
		double acquirePeriod = value;
		setDoubleParam(function, acquirePeriod);
	}
	else if (function == LAMBDA_EnergyThreshold)
	{
		std::vector<double> thresholds = det->thresholds();
		
		thresholds[0] = value;
		
		det->setThresholds(thresholds);
		this->getThresholds();
	}
	else if (function == LAMBDA_DualThreshold)
	{
		std::vector<double> thresholds = det->thresholds();
		
		thresholds[1] = value;
		
		det->setThresholds(thresholds);
		this->getThresholds();
	}
	else 
	{
		if (function < LAMBDA_FIRST_PARAM) 
		{
			status = ADDriver::writeFloat64(pasynUser, value);
		}
	}
	
	callParamCallbacks();
	return (asynStatus) status;
}

void ADLambda::getThresholds()
{
	std::vector<double> thresholds = det->thresholds();
	
	setDoubleParam(LAMBDA_EnergyThreshold, thresholds[0]);
	setDoubleParam(LAMBDA_DualThreshold, thresholds[1]);
}

void ADLambda::setTriggerMode(int mode)
{
	if      (mode == 0)    { det->setTriggerMode(xsp::lambda::TrigMode::SOFTWARE); }
	else if (mode == 1)    { det->setTriggerMode(xsp::lambda::TrigMode::EXT_SEQUENCE); }
	else if (mode == 2)    { det->setTriggerMode(xsp::lambda::TrigMode::EXT_FRAMES); }
}

void ADLambda::setOperatingMode(int mode)
{
	if (mode == 0)
	{
		det->setOperationMode(xsp::lambda::OperationMode(xsp::lambda::BitDepth::DEPTH_1));
		setIntegerParam(NDDataType, NDUInt8);
	}
	else if (mode == 1)
	{
		det->setOperationMode(xsp::lambda::OperationMode(xsp::lambda::BitDepth::DEPTH_6));
		setIntegerParam(NDDataType, NDUInt8);
	}
	else if (mode == 2)
	{
		det->setOperationMode(xsp::lambda::OperationMode(xsp::lambda::BitDepth::DEPTH_12));
		setIntegerParam(NDDataType, NDUInt16);
	}
	else if(mode == 3) 
	{
		det->setOperationMode(xsp::lambda::OperationMode(xsp::lambda::BitDepth::DEPTH_24));
		setIntegerParam(NDDataType, NDUInt32);
	}
	else if (mode == 4)
	{
		det->setOperationMode(xsp::lambda::OperationMode(xsp::lambda::BitDepth::DEPTH_1,
		                                                 xsp::lambda::ChargeSumming::OFF,
		                                                 xsp::lambda::CounterMode::DUAL));
		setIntegerParam(NDDataType, NDUInt8);
	}
	else if (mode == 5)
	{
		det->setOperationMode(xsp::lambda::OperationMode(xsp::lambda::BitDepth::DEPTH_6,
		                                                 xsp::lambda::ChargeSumming::OFF,
		                                                 xsp::lambda::CounterMode::DUAL));
		setIntegerParam(NDDataType, NDUInt8);
	}
	else if (mode == 6)
	{
		det->setOperationMode(xsp::lambda::OperationMode(xsp::lambda::BitDepth::DEPTH_12, 
		                                                 xsp::lambda::ChargeSumming::OFF,
		                                                 xsp::lambda::CounterMode::DUAL));
		setIntegerParam(NDDataType, NDUInt16);

	}
	else if (mode == 7)
	{
		det->setOperationMode(xsp::lambda::OperationMode(xsp::lambda::BitDepth::DEPTH_24, 
		                                                 xsp::lambda::ChargeSumming::OFF,
		                                                 xsp::lambda::CounterMode::DUAL));
		setIntegerParam(NDDataType, NDUInt32);
		
	}
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


	if (function == ADAcquire)
	{
		if (value && (adStatus == ADStatusIdle) && (adacquiring == 0))    { this->startAcquireEvent->trigger(); }
		else if (!value && adacquiring)
		{ 
			det->stopAcquisition();
			this->setIntegerParam(ADStatus, ADStatusAborting);
		}
		
		/** Make sure that we write the value to the param */
		setIntegerParam(addr, function, value);
		
		this->callParamCallbacks(addr);
		return asynSuccess;
	}

	if ((adStatus != ADStatusIdle) || adacquiring)    
	{ 
		setStringParam(ADStatusMessage, "Failed to set value, currently not Idle");
		this->callParamCallbacks();
		return asynError; 
	}
	else
	{
		setStringParam(ADStatusMessage, "");
	}
	
	/** Make sure that we write the value to the param */
	setIntegerParam(addr, function, value);
	
	if (function == ADNumImages)
	{
		det->setFrameCount(value); 
	}
	else if (function == ADTriggerMode)
	{
		this->setTriggerMode(value);
	}
	else if ((function == ADSizeX) || (function == ADSizeY) ||
	         (function == ADMinX)  || (function == ADMinY) )
	{
		this->setSizes();
	}
	else if (function == LAMBDA_OperatingMode) 
	{
		this->setOperatingMode(value);		
		this->setSizes();
	}
	else if (function < LAMBDA_FIRST_PARAM) 
	{
		status = ADDriver::writeInt32(pasynUser, value);
	}

	callParamCallbacks(addr);
	return (asynStatus) status;
}

/**
 * Override from super class to handle detector specific parameters.
 * If the parameter is from one of the super classes and is not handled
 * here, then pass along to ADDriver (direct super class)
 */
asynStatus ADLambda::writeOctet(asynUser* pasynUser, const char *value,
                                size_t nChars, size_t *nActual)
{
	int status = asynSuccess;
	int function = pasynUser->reason;

	if (function < LAMBDA_FIRST_PARAM) 
	{
		status = ADDriver::writeOctet(pasynUser, value, nChars, nActual);
	}
	
	callParamCallbacks();
	return (asynStatus) status;
}


/**
 * Override from superclass to handle cases for detector specific Parameters.
 * If the parameter is from one of the super classes and is not handled
 * here, then pass along to ADDriver (direct super class)
 */
asynStatus 	ADLambda::readOctet (asynUser *pasynUser, char *value, 
                                 size_t maxChars, size_t *nActual, int *eomReason)
{
	int status = asynSuccess;
	int function = pasynUser->reason;

	if (function < LAMBDA_FIRST_PARAM) 
	{
		status = ADDriver::readOctet(pasynUser, value, maxChars, nActual, eomReason);
	}
	
	callParamCallbacks();
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
