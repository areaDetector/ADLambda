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
	this->incrementValue(LAMBDA_ReadoutThreads);
	this->callParamCallbacks();

	acquire_data* data = new acquire_data;

	data->driver = this;
	data->receiver = receiver;

	epicsThreadCreate("ADLambda::acquireThread()",
              epicsThreadPriorityHigh,
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
	void LambdaConfig(const char *portName, const char* configPath, int numModules, int fake) 
	{
		new ADLambda(portName, configPath, numModules, fake);
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
ADLambda::ADLambda(const char *portName, const char *configPath, int numModules, int fake) :
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
	this->stopAcquireEvent = new epicsEvent();
	this->dequeLock = new epicsMutex();
	this->fake = fake;

	this->threadFinishEvents = (epicsEvent**) calloc(numModules, sizeof(epicsEvent*));
	
	for (int index = 0; index < numModules; index += 1)
	{
		this->threadFinishEvents[index] = new epicsEvent();
	}


	/* *************
	 * STRING PARAMS
	 * *************   
	 */
	 
	createParam( LAMBDA_ConfigFilePathString,    asynParamOctet,   &LAMBDA_ConfigFilePath);
	
	setStringParam(ADManufacturer, "X-Spectrum GmbH");
	setStringParam(LAMBDA_ConfigFilePath, configPath);
	
	// Write version to appropriate parameter
	setStringParam(NDDriverVersion, GIT_VERSION);
	
	/* *************
	 * DOUBLE PARAMS
	 * *************
	 */
	 
	createParam( LAMBDA_EnergyThresholdString,   asynParamFloat64, &LAMBDA_EnergyThreshold);
	createParam( LAMBDA_DualThresholdString,     asynParamFloat64, &LAMBDA_DualThreshold);
	
	setDoubleParam(LAMBDA_EnergyThreshold, 0.0);
	setDoubleParam(LAMBDA_DualThreshold, 0.0);
	
	
	/* **************
	 * INTEGER PARAMS
	 * **************
	 */
	
	createParam( LAMBDA_DecoderDetectedString,   asynParamInt32,   &LAMBDA_DecoderDetected);
	createParam( LAMBDA_DecodedQueueDepthString, asynParamInt32,   &LAMBDA_DecodedQueueDepth);
	createParam( LAMBDA_OperatingModeString,     asynParamInt32,   &LAMBDA_OperatingMode);
	createParam( LAMBDA_DualModeString,          asynParamInt32,   &LAMBDA_DualMode);
	createParam( LAMBDA_ChargeSummingString,     asynParamInt32,   &LAMBDA_ChargeSumming);
	createParam( LAMBDA_GatingEnableString,      asynParamInt32,   &LAMBDA_GatingEnable);
	createParam( LAMBDA_BadFrameCounterString,   asynParamInt32,   &LAMBDA_BadFrameCounter);
	createParam( LAMBDA_BadImageString,          asynParamInt32,   &LAMBDA_BadImage);
	createParam( LAMBDA_ReadoutThreadsString,    asynParamInt32,   &LAMBDA_ReadoutThreads);
	createParam( LAMBDA_StitchWidthString,       asynParamInt32,   &LAMBDA_StitchedWidth);
	createParam( LAMBDA_StitchHeightString,      asynParamInt32,   &LAMBDA_StitchedHeight);
	
	setIntegerParam(LAMBDA_DecoderDetected, 0);
	setIntegerParam(LAMBDA_DecodedQueueDepth, 0);
	setIntegerParam(LAMBDA_OperatingMode, 0);
	setIntegerParam(LAMBDA_DualMode, 0);
	setIntegerParam(LAMBDA_ChargeSumming, 0);
	setIntegerParam(LAMBDA_GatingEnable, 0);
	setIntegerParam(LAMBDA_BadFrameCounter, 0);
	setIntegerParam(LAMBDA_BadImage, 0);
	setIntegerParam(LAMBDA_ReadoutThreads, 0);
	setIntegerParam(LAMBDA_StitchedWidth, 0);
	setIntegerParam(LAMBDA_StitchedHeight, 0);
	
	
	this->connect();
}

ADLambda::~ADLambda()    { this->disconnect(); }

asynStatus ADLambda::connect()
{
	this->disconnect();
	this->setIntegerParam(ADStatus, ADStatusInitializing);
   
	this->tryConnect();
	                  
	return asynSuccess;
}


void ADLambda::tryConnect()
{
	std::string error_msg;

	while (! this->connected)
	{
		try
		{
			this->sys = xsp::createSystem(this->configFileName);
			
			if (this->sys == nullptr) { throw xsp::RuntimeError("Couldn't open config file", xsp::StatusCode::BAD_RESOURCE_UNAVAILABLE); }

			this->sys->connect();
			this->sys->initialize();
			
			/**
			 * Set up Detector
			 */
			
			this->det = std::dynamic_pointer_cast<xsp::lambda::Detector>(sys->detector("lambda"));

			this->det->setEventHandler([&](auto t, const void* d) {
				switch (t) 
				{
					case xsp::EventType::READY:
						break;
						
					case xsp::EventType::START:
						break;
						
					case xsp::EventType::STOP:
						break;
				}
				
				this->callParamCallbacks();
			});
			
			xsp::setLogHandler([](xsp::LogLevel l, const std::string& m) {
				switch (l) {
					case xsp::LogLevel::ERROR:
						printf("Lambda Driver Error: %s\n", m.c_str());
						break;
					
					case xsp::LogLevel::WARN:
						printf("Lambda Driver Warning: %s\n", m.c_str());
						break;
					case xsp::LogLevel::INFO:
						printf("Lambda Driver Info: %s\n", m.c_str());
						break;
					case xsp::LogLevel::DEBUG:
						//printf("Lambda Driver Debug: %s\n", m.c_str());
						break;
					default:
						printf("Lambda Driver Unknown: %s\n", m.c_str());
						break;
				}
			});


			/*
			 * Set up reception of images
			 *
			 * Check to see if there is a stitching decoder enabled, 
			 * otherwise connect to modules individually
			 */

			if (sys->postDecoderIds().size() >= 1)
			{
				this->setIntegerParam(LAMBDA_DecoderDetected, 1);
				this->hasDecoder = true;
				this->inputs.push_back(sys->postDecoder("lambda"));
			}
			else
			{
				for (auto ID : sys->receiverIds())
				{
					auto rec = std::dynamic_pointer_cast<xsp::lambda::Receiver>(sys->receiver(ID));

					printf("Waiting for RAM allocation for receiver %s\n", ID.c_str());
					while (!rec->ramAllocated()) { epicsThreadSleep(SHORT_TIME); }
				
					inputs.push_back(rec);
				}
			}

            for (int index = 1; index <= det->numberOfModules(); index += 1) 
            {
                printf("Waiting for HV to settle on module %d...\n", index);
                while (!det->voltageSettled(index)) epicsThreadSleep(SHORT_TIME);
            }

			this->connected = true;
			
		}
		catch  (const xsp::RuntimeError& e)
		{
			error_msg = std::string(e.what());

			this->setStringParam(ADStatusMessage, error_msg.c_str());
			this->callParamCallbacks();
			epicsThreadSleep(SHORT_TIME);
		}
	}

	this->readParameters();
	this->setIntegerParam(ADStatus, ADStatusIdle);
	this->callParamCallbacks();
	
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

void ADLambda::readParameters()
{
	int full_width = 0, full_height = 0;

	for (auto inp : this->inputs)
	{	
		if (this->hasDecoder)
		{
			full_height = std::get<1>(inp)->frameHeight();
			full_width  = std::get<1>(inp)->frameWidth();
		}
		else
		{
			auto temp = std::get<0>(inp);
			full_width  = std::max(full_width,  (int)(temp->frameWidth() + temp->position().x));
			full_height = std::max(full_height, (int)(temp->frameHeight() + temp->position().y));
		}
	}
	
	setIntegerParam(LAMBDA_StitchedHeight, full_height);
	setIntegerParam(LAMBDA_StitchedWidth, full_width);
	
	this->setSizes();
	
	xsp::lambda::OperationMode mode = this->det->operationMode();
	xsp::lambda::Gating gate = this->det->gatingMode();
	xsp::lambda::TrigMode trig = this->det->triggerMode();
	
	// Operating Mode
	if      (mode.bit_depth == xsp::lambda::BitDepth::DEPTH_1)          { this->writeDepth(ONE_BIT); }
	else if (mode.bit_depth == xsp::lambda::BitDepth::DEPTH_6)          { this->writeDepth(SIX_BIT); }
	else if (mode.bit_depth == xsp::lambda::BitDepth::DEPTH_12)         { this->writeDepth(TWELVE_BIT); }
	else if (mode.bit_depth == xsp::lambda::BitDepth::DEPTH_24)         { this->writeDepth(TWENTY_FOUR_BIT); }
	
	if      (mode.counter_mode == xsp::lambda::CounterMode::SINGLE)     { setIntegerParam(LAMBDA_DualMode, 0); }
	else if (mode.counter_mode == xsp::lambda::CounterMode::DUAL)       { setIntegerParam(LAMBDA_DualMode, 1); }
	
	if      (mode.charge_summing == xsp::lambda::ChargeSumming::OFF)    { setIntegerParam(LAMBDA_ChargeSumming, 0); }
	else if (mode.charge_summing == xsp::lambda::ChargeSumming::ON)     { setIntegerParam(LAMBDA_ChargeSumming, 1); }
	
	if      (gate == xsp::lambda::Gating::OFF)                          { setIntegerParam(LAMBDA_GatingEnable, 0); }
	else if (gate == xsp::lambda::Gating::ON)                           { setIntegerParam(LAMBDA_GatingEnable, 1); }
	
	if      (trig == xsp::lambda::TrigMode::SOFTWARE)                   { setIntegerParam(ADTriggerMode, 0); }
	else if (trig == xsp::lambda::TrigMode::EXT_SEQUENCE)               { setIntegerParam(ADTriggerMode, 1); }
	else if (trig == xsp::lambda::TrigMode::EXT_FRAMES)                 { setIntegerParam(ADTriggerMode, 2); }

	std::string model = sys->id();
	setStringParam(ADModel, model);

	std::string fwVers = det->firmwareVersion(1);
	setStringParam(ADFirmwareVersion, fwVers);

	std::string version = xsp::libraryVersion();
	setStringParam(ADSDKVersion, version);
		
	callParamCallbacks();
}


void ADLambda::sendParameters()
{
	setStringParam(ADStatusMessage, "Sending settings to Detector");
	callParamCallbacks();

	double shuttertime, low_energy, high_energy;
	int trigger, operation, dual, charge, frames, gate;
	
	getDoubleParam(ADAcquireTime, &shuttertime);
	getDoubleParam(LAMBDA_EnergyThreshold, &low_energy);
	getDoubleParam(LAMBDA_DualThreshold, &high_energy);
	
	getIntegerParam(ADTriggerMode, &trigger);
	getIntegerParam(LAMBDA_OperatingMode, &operation);
	getIntegerParam(LAMBDA_DualMode, &dual);
	getIntegerParam(LAMBDA_ChargeSumming, &charge);
	getIntegerParam(LAMBDA_GatingEnable, &gate);
	getIntegerParam(ADNumImages, &frames);
	
	
	// Set Default Values
	xsp::lambda::Gating         gm =    xsp::lambda::Gating::OFF;
	xsp::lambda::TrigMode       tm =    xsp::lambda::TrigMode::SOFTWARE;
	xsp::lambda::CounterMode    cm =    xsp::lambda::CounterMode::SINGLE;
	xsp::lambda::BitDepth       depth = xsp::lambda::BitDepth::DEPTH_1;
	xsp::lambda::ChargeSumming  sum =   xsp::lambda::ChargeSumming::OFF;
	
	if (dual)    { cm = xsp::lambda::CounterMode::DUAL; }
	if (charge)  { sum = xsp::lambda::ChargeSumming::ON; }
	if (gate)    { gm = xsp::lambda::Gating::ON; }
	
	// Trigger Mode
	if      (trigger == 1)    { tm = xsp::lambda::TrigMode::EXT_SEQUENCE; }
	else if (trigger == 2)    { tm = xsp::lambda::TrigMode::EXT_FRAMES; }
	
	// Operating Mode
	if      (operation == ONE_BIT)         { depth = xsp::lambda::BitDepth::DEPTH_1; }
	else if (operation == SIX_BIT)         { depth = xsp::lambda::BitDepth::DEPTH_6; }
	else if (operation == TWELVE_BIT)      { depth = xsp::lambda::BitDepth::DEPTH_12; }
	else if (operation == TWENTY_FOUR_BIT) { depth = xsp::lambda::BitDepth::DEPTH_24; }
	
	xsp::lambda::OperationMode om_set(depth, sum, cm);
	xsp::lambda::OperationMode om_get = det->operationMode();
	
	// Set Values
	if (det->gatingMode() != gm)    { det->setGatingMode(gm); }
	if (det->triggerMode() != tm)     { det->setTriggerMode(tm); }
	
	if (om_get.bit_depth != om_set.bit_depth ||
	    om_get.charge_summing != om_set.charge_summing ||
	    om_get.counter_mode != om_set.counter_mode)   
	{ 
		det->setOperationMode(om_set); 
	}
	
	if (std::abs(det->shutterTime() - (shuttertime * 1000.0)) >= 0.00001)
	{
		det->setShutterTime(shuttertime * 1000);
	}
	
	// Set Thresholds
	std::vector<double> thresholds = det->thresholds();
	
	thresholds.reserve(2);
	
	if (std::abs(thresholds[0] - low_energy) >= 0.00001 || 
        ((dual || charge) && (std::abs(thresholds[1] - high_energy) >= 0.00001)))
    {
        if (dual || charge) {
            printf("Setting thresholds: %f keV, %f keV\n", low_energy, high_energy);
            det->setThresholds(std::vector<double>{low_energy, high_energy});
        } else {
                printf("Setting threshold: %f keV\n", low_energy);
                det->setThresholds(std::vector<double>{low_energy});
        }
    }
		
	if (det->frameCount() != frames)    { det->setFrameCount(frames); }
	
	this->setSizes();
	
	setStringParam(ADStatusMessage, "");
	this->callParamCallbacks();
}

bool ADLambda::tryStartAcquire()
{
	try
	{ 
		while(! det->isReady())    { epicsThreadSleep(SHORT_TIME); }
		this->det->startAcquisition();
		return true;
	}
	catch (const xsp::RuntimeError& e)
	{
		std::string message(e.what());
		
		this->setStringParam(ADStatusMessage, message.c_str());
		this->callParamCallbacks();
		
		return false;
	}
}

bool ADLambda::tryStopAcquire()
{
	try
	{
		this->det->stopAcquisition();
		return true;
	}
	catch (const xsp::RuntimeError& e)
	{
		std::string message(e.what());

		this->setStringParam(ADStatusMessage, message.c_str());
		this->callParamCallbacks();

		return false;
	}
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
			bool signal = this->startAcquireEvent->wait(SHORT_TIME);
		this->lock();
		
		if (!signal)    { continue; }
		
		bool aborted = false;
		
		this->unlock();
			this->setStringParam(ADStatusMessage, "Waiting for modules to be ready");
			while(! det->isReady())    { aborted = this->stopAcquireEvent->wait(SHORT_TIME); }
		this->lock();
		
		if (aborted)    { continue; }
		
		// Sync epics parameters to detector
		try
		{
			this->sendParameters();
		}
		catch  (const xsp::RuntimeError& e)
		{
			std::string error_msg(e.what());

			this->setStringParam(ADStatusMessage, error_msg.c_str());
			this->callParamCallbacks();
			continue;
		}

		this->setIntegerParam(LAMBDA_BadImage, 0);
		this->setIntegerParam(ADStatus, ADStatusWaiting);
		this->callParamCallbacks();
		
		// Attempt to start aquisition, allow user to abort acquisition
		while (! aborted && ! this->tryStartAcquire())
		{
			this->unlock();
				aborted = this->stopAcquireEvent->wait(SHORT_TIME);
			this->lock();
		}
		
		this->setStringParam(ADStatusMessage, "");
		
		// If stop is pressed, reset to idle
		if (aborted)
		{ 
			this->setIntegerParam(ADAcquire, 0);
			this->setIntegerParam(ADStatus, ADStatusIdle);
			this->callParamCallbacks();
			continue; 
		}
		
		this->setIntegerParam(ADStatus, ADStatusAcquire);
		this->callParamCallbacks();
		
		// Spawn acquisition threads
		for (size_t inp_index = 0; inp_index < this->inputs.size(); inp_index += 1)
		{
			this->setIntegerParam(inp_index, ADNumImagesCounter, 0);
			this->setIntegerParam(inp_index, LAMBDA_BadFrameCounter, 0);
			this->callParamCallbacks(inp_index);
			
			this->spawnAcquireThread(inp_index);
		}
		
		// Wait for all threads to finish acquiring
		for (size_t index = 0; index < this->inputs.size(); index += 1)
		{
			this->unlock();
				this->threadFinishEvents[index]->wait();
			this->lock();
			
			decrementValue(LAMBDA_ReadoutThreads);
			callParamCallbacks();
		}
		
		this->frames.clear();

		this->setIntegerParam(ADAcquire, 0);
		this->setIntegerParam(ADStatus, ADStatusReadout);
		this->callParamCallbacks();
		
		while (! export_queue.empty())    
		{
			this->unlock();
			epicsThreadSleep(SHORT_TIME); 
			this->lock();
		}
		
		this->setIntegerParam(ADStatus, ADStatusIdle);
		this->callParamCallbacks();
	}
	
	this->unlock();
}

/**
 * Thread to pull and export NDArrays being generated by the acquisition threads
 */
void ADLambda::exportThread()
{	
	while(this->connected)
	{
		while (export_queue.empty())    { epicsThreadSleep(SHORT_TIME); }
		
		if (this->pImage)    { this->pImage->release(); }
		
		// Pull from available frames
		dequeLock->lock();
			this->pImage = export_queue.front();
			export_queue.pop_front();
		dequeLock->unlock();
		
		NDArrayInfo info;
		this->pImage->getInfo(&info);
		
		
		this->lock();
			incrementValue(NDArrayCounter);
			this->setIntegerParam(NDArraySize, info.totalBytes);
		
			int arrayCallbacks;
			getIntegerParam(NDArrayCallbacks, &arrayCallbacks);
		
			this->callParamCallbacks();
			if (arrayCallbacks)    { doCallbacksGenericPointer(this->pImage, NDArrayData, 0); }
		this->unlock();
	}
}


/**
 * Thread spawned per detector module, acquires frames from indexed receiver and
 * copies the data to the correct spot in the stitched image.
 */
void ADLambda::acquireThread(int index)
{
	lambda_input input = this->inputs[index];

	int width, height, toRead, datatype, dual_mode, depth;
	double exposure;
	
	/**
	 * Grab parameters that will be used for the entirety of the acquisition
	 */
	
	this->lock();
		this->getIntegerParam(ADMaxSizeX, &width);
		this->getIntegerParam(ADMaxSizeY, &height);
		this->getIntegerParam(ADNumImages, &toRead);
		this->getIntegerParam(NDDataType, &datatype);
		this->getIntegerParam(LAMBDA_OperatingMode, &depth);
		this->getIntegerParam(LAMBDA_DualMode, &dual_mode);
		this->getDoubleParam(ADAcquireTime, &exposure);
	this->unlock();
	
	size_t imagedims_output[2] = { (size_t) width, (size_t) height};
	const int frame_width  = std::visit([](auto&& arg) -> const int { return arg->frameWidth();  }, input);
	const int frame_height = std::visit([](auto&& arg) -> const int { return arg->frameHeight(); }, input);
	int x_shift = 0;
	int y_shift = 0;
	
	if (! this->hasDecoder)
	{
		xsp::Position modulepos = std::get<0>(input)->position();
		x_shift = (int) modulepos.x;
		y_shift = (int) modulepos.y;
	}
	
	xsp::Frame* acquired[2] = { NULL, NULL };
	
	NDArrayInfo info;
	
	int numAcquired = 0;
	int dual = 0;
	
	while (numAcquired < toRead)
	{
		acquired[dual] = std::visit([](auto&& arg) -> xsp::Frame* { return (xsp::Frame*) arg->frame(1500); }, input);
		
		// Empty frame plus the detector saying it's not busy means something's gone wrong.
		if (acquired[dual] == nullptr || acquired[dual]->data() == NULL)
		{
			if (det->isBusy())    { continue; }
			else                  { this->tryStopAcquire(); break; }
		}
		
		/*
		 * For dual mode, every acquisition is two frames, increment dual to save frame in
		 * the second slot of acquired.
		 */
		if (dual_mode && !dual)    { dual = 1; continue; }
		else                       { dual = 0; }
	
		const int frame_no = acquired[0]->nr();
		
		NDArray* output;
		
		this->lock();
		
			// If there's not an NDArray stored for this frame_no, create one
			if (this->hasDecoder)
			{
				output = pNDArrayPool->alloc(2, imagedims_output, (NDDataType_t) datatype, 0, NULL);
				output->uniqueId = frame_no;
				output->getInfo(&info);
				
				updateTimeStamp(&(output->epicsTS));
				output->timeStamp = output->epicsTS.secPastEpoch + output->epicsTS.nsec / ONE_BILLION;
			
				memset((char*) output->pData, 0, imagedims_output[0] * imagedims_output[1] * info.bytesPerElement);
				
				incrementValue(ADNumImagesCounter);
			}
			else if (this->frames.find(frame_no) == this->frames.end())
			{
				NDArray* new_frame = pNDArrayPool->alloc(2, imagedims_output, (NDDataType_t) datatype, 0, NULL);
				new_frame->uniqueId = 0;
				new_frame->getInfo(&info);
				updateTimeStamp(&(new_frame->epicsTS));
				new_frame->timeStamp = new_frame->epicsTS.secPastEpoch + new_frame->epicsTS.nsec / ONE_BILLION;
			
				memset((char*) new_frame->pData, 0, imagedims_output[0] * imagedims_output[1] * info.bytesPerElement);

				this->frames[frame_no] = new_frame;
				output = new_frame;
				incrementValue(ADNumImagesCounter);
			}
			else
			{
				output = this->frames[frame_no];
			}
		this->unlock();
		
		output->getInfo(&info);
		numAcquired += 1;
		
		// If not in dual mode, will just take the first status twice
		int bad_frame = ((int) acquired[0]->status() | (int) acquired[dual_mode]->status()); 
		
		// Stitch frame into its correct spot in the NDArray
		if (bad_frame == (int) xsp::FrameStatusCode::FRAME_OK)
		{
			if (! this->fake)
			{
				for (int which = 0; which <= dual_mode; which += 1)
				{
					char* in_data = (char*) acquired[which]->data();
					char* out_data = (char*) output->pData;

					if (this->hasDecoder)
					{
						int in_offset = 0;
						int out_offset = frame_height * which * width * info.bytesPerElement;
					
						std::memcpy(&out_data[out_offset], &in_data[in_offset], frame_height * frame_width * info.bytesPerElement);
					}
					else
					{
						for (int row = 0; row < frame_height; row += 1)
						{
							int in_offset = row * frame_width * info.bytesPerElement;
							int out_offset = ((y_shift + row + (frame_height * which)) * width + x_shift) * info.bytesPerElement;
						
							std::memcpy(&out_data[out_offset], &in_data[in_offset], frame_width * info.bytesPerElement);
						}
					}
				}
			}
		}
		
		std::visit([&acquired](auto&& arg){ arg->release(acquired[0]); }, input);
		if (dual_mode)    { std::visit([&acquired](auto&& arg){ arg->release(acquired[1]); }, input); }

		
		this->lock();
			if (! this->hasDecoder)
			{
				if (bad_frame)
				{ 
					incrementValue(LAMBDA_BadFrameCounter); 
					output->release();
				}
				else
				{
					dequeLock->lock();
						this->export_queue.push_back(output); 
					dequeLock->unlock();
				}
			}
			else
			{
				int id = output->uniqueId;
					
				/*
				 * Negative value means that one of the threads reported a bad frame,
				 * magnitude is the number of threads that have reported frames.
				 */
				if (id < 0)                { id -= 1; }
				else if (bad_frame > 0)    { id = -1 * std::abs(id) - 1; }
				else                       { id += 1; }
			
				if   (std::abs(id) < this->inputs.size())    { output->uniqueId = id; }
				else 
				{
					output->uniqueId = frame_no;
					
					if (id < 0)    
					{ 
						incrementValue(LAMBDA_BadFrameCounter); 
						output->release();
					}	
					else
					{ 
						dequeLock->lock();
							this->export_queue.push_back(output); 
						dequeLock->unlock();
					}
					
					this->frames.erase(frame_no);
				}
			}
		
			int numBuffered = std::visit([](auto&& arg) -> int { return arg->framesQueued(); }, input);
			this->setIntegerParam(index, LAMBDA_DecodedQueueDepth, numBuffered);
		
		this->unlock();
	}
	
	this->threadFinishEvents[index]->trigger();
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
		if (value && (adStatus == ADStatusIdle))    
		{ 
			this->setIntegerParam(ADStatus, ADStatusAcquire);
			this->callParamCallbacks();
			this->startAcquireEvent->trigger(); 
		}
		else if (!value && (adStatus != ADStatusIdle))
		{  
			this->setIntegerParam(ADStatus, ADStatusAborting);
			this->callParamCallbacks();
			
			this->tryStopAcquire();
			if (adStatus == ADStatusWaiting)    { this->stopAcquireEvent->trigger(); }
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
static const iocshArg LambdaConfigArg3 = { "fake", iocshArgInt };
static const iocshArg * const LambdaConfigArgs[] = { &LambdaConfigArg0, &LambdaConfigArg1, &LambdaConfigArg2, &LambdaConfigArg3};

static void configLambdaCallFunc(const iocshArgBuf *args) {
	LambdaConfig(args[0].sval, args[1].sval, args[2].ival, args[3].ival);
}
static const iocshFuncDef configLambda = { "LambdaConfig", 4, LambdaConfigArgs };

static void LambdaRegister(void) 
{
	iocshRegister(&configLambda, configLambdaCallFunc);
}

extern "C" 
{
	epicsExportRegistrar(LambdaRegister);
}
