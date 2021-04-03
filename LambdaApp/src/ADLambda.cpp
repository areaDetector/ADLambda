/**
 Copyright (c) 2015, UChicago Argonne, LLC
 See LICENSE file.
 */
/* ADLambda.cpp */
#include <algorithm>
#include <iocsh.h>
#include <vector>
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
static void monitor_thread_callback(void *drvPvt)    { ((ADLambda*) drvPvt)->monitorThread(); }

static void receiver_acquire_callback(void *drvPvt)
{
	acquire_data* data = (acquire_data*) drvPvt;
	
	data->driver->acquireThread(data->receiver, data->thread_no);
	
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
	 * \param[in] readout Number of readout threads per module
	 */
	void LambdaConfig(const char *portName, const char* configPath, int numModules, int readout) 
	{
		new ADLambda(portName, configPath, numModules, readout);
	}

}

const char *ADLambda::driverName = "Lambda";

/**
 * Constructor
 * \param[in] portName The name of the asyn port driver to be created.
 * \param[in] configPath directory containing configuration file location
 * \param[in] maxBuffers The maximum number of NDArray buffers that the
 *            NDArrayPool for this driver is
 *            allowed to allocate. Set this to -1 to allow an unlimited number
 *            of buffers.
 * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for
 *            this driver is allowed to allocate. Set this to -1 to allow an
 *            unlimited amount of memory.
 * \param[in] priority The thread priority for the asyn port driver thread if
 *            ASYN_CANBLOCK is set in asynFlags.
 * \param[in] stackSize The stack size for the asyn port driver thread if
 *            ASYN_CANBLOCK is set in asynFlags.
 *
 */
ADLambda::ADLambda(const char *portName, const char *configPath, int numModules, int readout) :
	ADDriver(portName, 
	         numModules, 
	         int(NUM_LAMBDA_PARAMS), 
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
	this->ReadThreadPerModule = readout;

	this->startAcquireEvent = new epicsEvent();

	this->saved_frames = calloc(numModules * this->ReadThreadPerModule, sizeof(NDArray*));
	this->threadFinishEvents = calloc(numModules * this->ReadThreadPerModule, sizeof(epicsEvent*));
	this->threadReceiverLocks = calloc(numModules, sizeof(epicsMutex*));
	
	for (int index = 0; index < numModules * this->ReadThreadPerModule; index += 1)
	{ 
		this->saved_frames[index] = nullptr;
		this->threadFinishEvents[index] = new epicsEvent();
	}
	
	for (int index = 0; index < numModules; index += 1)    { this->threadReceiverLocks[index] = new epicsMutex(); }

	createParam(LAMBDA_VersionNumberString, asynParamOctet, &LAMBDA_VersionNumber);
	createParam(LAMBDA_ConfigFilePathString, asynParamOctet, &LAMBDA_ConfigFilePath);
	createParam(LAMBDA_EnergyThresholdString, asynParamFloat64, &LAMBDA_EnergyThreshold);
	createParam(LAMBDA_EnergyThresholdRBVString, asynParamFloat64, &LAMBDA_EnergyThresholdRBV);
	createParam(LAMBDA_DualThresholdString, asynParamFloat64, &LAMBDA_DualThreshold);
	createParam(LAMBDA_DualThresholdRBVString, asynParamFloat64, &LAMBDA_DualThresholdRBV);
	createParam(LAMBDA_DecodedQueueDepthString, asynParamInt32, &LAMBDA_DecodedQueueDepth);
	createParam(LAMBDA_OperatingModeString, asynParamInt32, &LAMBDA_OperatingMode);
	createParam(LAMBDA_DetectorStateString, asynParamInt32, &LAMBDA_DetectorState);
	createParam(LAMBDA_BadFrameCounterString, asynParamInt32, &LAMBDA_BadFrameCounter);
	createParam(LAMBDA_MedipixIDsString, asynParamOctet, &LAMBDA_MedipixIDs);
	createParam(LAMBDA_DetCoreVersionNumberString, asynParamOctet, &LAMBDA_DetCoreVersionNumber);
	createParam(LAMBDA_BadImageString, asynParamInt32, &LAMBDA_BadImage);
	createParam(LAMBDA_ReadoutThreadsString, asynParamInt32, &LAMBDA_ReadoutThreads);
	
	setIntegerParam(LAMBDA_ReadoutThreads, 0);
	
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
	
	std::vector<std::string> IDS = sys->receiverIds();
	
	for (int index = 0; index < IDS.size(); index += 1)
	{
		std::shared_ptr<xsp::Receiver> rec = sys->receiver(IDS[index]);
		
		setIntegerParam(index, ADMaxSizeX, rec->frameWidth());
		setIntegerParam(index, ADMaxSizeY, rec->frameHeight());
		setIntegerParam(index, ADMinX, 0);
		setIntegerParam(index, ADMinY, 0);
		setIntegerParam(index, ADSizeX, rec->frameWidth());
		setIntegerParam(index, ADSizeY, rec->frameHeight());
		setIntegerParam(index, NDArraySizeX, rec->frameWidth());
		setIntegerParam(index, NDArraySizeY, rec->frameHeight());
		setIntegerParam(index, NDArraySize, 0);
		
		int depth = rec->frameDepth();
		
		if (depth == ONE_BIT)
		{
			setIntegerParam(index, NDDataType, NDUInt8);
			setIntegerParam(index, LAMBDA_OperatingMode, ONE_BIT_MODE);
		}
		else if (depth == SIX_BIT)
		{
			setIntegerParam(index, NDDataType, NDUInt8);
			setIntegerParam(index, LAMBDA_OperatingMode, SIX_BIT_MODE);
		}
		else if (depth == TWELVE_BIT) 
		{
			setIntegerParam(index, NDDataType, NDUInt16);
			setIntegerParam(index, LAMBDA_OperatingMode, TWELVE_BIT_MODE);
		}
		else if (depth == TWENTY_FOUR_BIT) 
		{
			setIntegerParam(index, NDDataType, NDUInt32);
			setIntegerParam(index, LAMBDA_OperatingMode, TWENTY_FOUR_BIT_MODE);
		}
		
		recs.push_back(rec);
	}

	std::string manufacturer = "X-Spectrum GmbH";
	setStringParam(ADManufacturer, manufacturer);

	std::string model = "Lambda 750K";

	//std::string moduleID = det->chipIds(1)[0];
	//setStringParam(ADSerialNumber, moduleID);

	//std::string fwVers = det->firmwareVersion(1);
	//setStringParam(ADFirmwareVersion, fwVers);

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
	                  
	epicsThreadCreate("ADLambda::monitorThread()",
	                  epicsThreadPriorityLow,
	                  epicsThreadGetStackSize(epicsThreadStackMedium),
	                  (EPICSTHREADFUNC)::monitor_thread_callback,
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

/**
 * Method called when WriteInt32 intercepts a message to stop
 * acquisition or if handleNewImageTask finds the requested
 * number of images have been captured.
 */
asynStatus ADLambda::acquireStop()
{
	int status = asynSuccess;

	det->stopAcquisition();

	setIntegerParam(ADStatus, ADStatusIdle);
	setIntegerParam(ADAcquire, 0);
	callParamCallbacks();

	return (asynStatus) status;
}

void ADLambda::spawnAcquireThread(int receiver, int thread_no)
{
	int curr_threads;
	
	getIntegerParam(LAMBDA_ReadoutThreads, &curr_threads);
	curr_threads += 1;
	setIntegerParam(LAMBDA_ReadoutThreads, curr_threads);
	callParamCallbacks();

	acquire_data* data = new acquire_data;

	data->driver = this;
	data->receiver = receiver;
	data->thread_no = thread_no;

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
		
		det->startAcquisition();
		
		this->unlock();
		
		for (int thread_index = 0; thread_index < this->ReadThreadPerModule; thread_index += 1)
		{
			for (int rec_index = 0; rec_index < this->recs.size(); rec_index += 1)
			{		
				this->spawnAcquireThread(rec_index, thread_index);
			}
		}
		
		int threads_started;
		
		getIntegerParam(LAMBDA_ReadoutThreads, &threads_started);
		
		int threads_running = threads_started;
		
		for (int index = 0; index < threads_started; index += 1)
		{
			this->threadFinishEvents[index]->wait();
			
			threads_running -= 1;

			setIntegerParam(LAMBDA_ReadoutThreads, threads_running);
			callParamCallbacks();
		}
		
		this->lock();
		
		acquireStop();
	}
	
	this->unlock();
}

void ADLambda::monitorThread()
{
}
		
void ADLambda::acquireThread(int receiver, int thread_no)
{
	int virtual_index = receiver + (this->recs.size() * thread_no);

    this->setIntegerParam(receiver, ADNumImagesCounter, 0);
	this->setIntegerParam(receiver, LAMBDA_BadFrameCounter, 0);
	this->setIntegerParam(receiver, LAMBDA_BadImage, 0);
	this->callParamCallbacks(receiver);

	int operating_mode;
	int width, height;
	
	this->getIntegerParam(LAMBDA_OperatingMode, &operating_mode);
	this->getIntegerParam(ADSizeX, &width);
	this->getIntegerParam(ADSizeY, &height);
	
	size_t imagedims[2] = {width, height};
	
	bool dual_mode = (operating_mode > TWENTY_FOUR_BIT_MODE);
	
	if (dual_mode)    { imagedims[1] = height * 2; }
	
	
	/*
	 * Each module will use address 0's number of images
	 * so that the user doesn't have to change the number
	 * of images for each module separately.
	 */
	int toRead;
	this->getIntegerParam(ADNumImages, &toRead);
	
	double exposure;
	this->getDoubleParam(ADAcquireTime, &exposure);
	
	
	std::shared_ptr<xsp::Receiver> rec = this->recs[receiver];
	
	xsp::Frame* frame = NULL;
	xsp::Frame* dual_frame = NULL;
	
	int numAcquired = 0;
	bool dual = false;
	
	// Wait for the frames to start coming in
	while (rec->framesQueued() == 0)
	{
		if (!det->isBusy())
		{
			this->threadFinishEvents[virtual_index]->trigger();
			return;
		}
	
		epicsThreadSleep(exposure); 
	}
	
	while (numAcquired < toRead)
	{
		this->callParamCallbacks(receiver);
	
		xsp::Frame* temp = rec->frame(1500);
		
		if (temp == nullptr || temp->data() == NULL)
		{ 
			if (det->isBusy())    { continue; }
			else                  { break; }
		}
		
		if (!dual)
		{ 
			frame = temp;
			
			if (dual_mode)    { dual = true; continue; } 
		}
		else
		{ 
			dual_frame = temp; 
			dual = false;
		}
		
		this->threadReceiverLocks[receiver]->lock();
			this->getIntegerParam(receiver, ADNumImagesCounter, &numAcquired);
			numAcquired += 1;
			this->setIntegerParam(receiver, ADNumImagesCounter, numAcquired);
		this->threadReceiverLocks[receiver]->unlock();
	
		if (frame->data() == NULL || frame->status() != xsp::FrameStatusCode::FRAME_OK ||
		   (dual_mode && (dual_frame->data() == NULL || dual_frame->status() != xsp::FrameStatusCode::FRAME_OK)))
		{
			int badFrames;
			
			this->threadReceiverLocks[receiver]->lock();
				this->getIntegerParam(receiver, LAMBDA_BadFrameCounter, &badFrames);
				badFrames += 1;
				this->setIntegerParam(receiver, LAMBDA_BadFrameCounter, badFrames);		
				this->setIntegerParam(receiver, LAMBDA_BadImage, 1);
			this->threadReceiverLocks[receiver]->unlock();
			
			rec->release(frame->nr());
			if (dual_mode)    { rec->release(dual_frame->nr()); }
			
			continue;
		}
	
		if (this->saved_frames[virtual_index])    { this->saved_frames[virtual_index]->release(); }
		
		
		int arrayCallbacks;
		getIntegerParam(NDArrayCallbacks, &arrayCallbacks);
		
		if (arrayCallbacks)
		{
			NDArray* output = pNDArrayPool->alloc(2, imagedims, this->imageDataType, 0, NULL);
			char* img_data = (char*) output->pData;
			
			this->saved_frames[virtual_index] = output;
			
			NDArrayInfo arrayInfo;
			output->getInfo(&arrayInfo);
			
			std::memset(img_data, 0, arrayInfo.totalBytes);
			
			xsp::Position modulepos = rec->position();
			output->dims[0].offset = (int) modulepos.x;
			output->dims[1].offset = (int) modulepos.y;
			
			memcpy(img_data, (char*) frame->data(), arrayInfo.totalBytes);
			
			if (dual_mode)
			{
				memcpy(&img_data[arrayInfo.totalBytes], (char*) dual_frame->data(), arrayInfo.totalBytes);
			}
						
			setIntegerParam(NDArraySize, arrayInfo.totalBytes);
			
			/* Time stamps */
			epicsTimeStamp currentTime;
			epicsTimeGetCurrent(&currentTime);
			output->timeStamp = currentTime.secPastEpoch + currentTime.nsec / ONE_BILLION;
			updateTimeStamp(&output->epicsTS);
			
			output->uniqueId = frame->nr();
			
			int arrayCounter;
			
			this->threadReceiverLocks[receiver]->lock();
				getIntegerParam(receiver, NDArrayCounter, &arrayCounter);
				arrayCounter += 1;
				setIntegerParam(receiver, NDArrayCounter, arrayCounter);
			this->threadReceiverLocks[receiver]->unlock();
			
			callParamCallbacks(receiver);
			doCallbacksGenericPointer(output, NDArrayData, receiver);
		}
		
		rec->release(frame->nr());
		if (dual_mode)    { rec->release(dual_frame->nr()); }
		
		
		int numBuffered = rec->framesQueued();
		this->setIntegerParam(receiver, LAMBDA_DecodedQueueDepth, numBuffered);
	}
	
	int numBuffered = rec->framesQueued();
	this->setIntegerParam(receiver, LAMBDA_DecodedQueueDepth, numBuffered);
	this->callParamCallbacks(receiver);
	
	this->threadFinishEvents[virtual_index]->trigger();
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
	
	setDoubleParam(LAMBDA_EnergyThresholdRBV, thresholds[0]);
	setDoubleParam(LAMBDA_DualThresholdRBV, thresholds[1]);
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
		if (value && (adStatus == ADStatusIdle))    { this->startAcquireEvent->trigger(); }
		else if (!value && adacquiring)
		{ 
			det->stopAcquisition();
			this->setIntegerParam(ADStatus, ADStatusAborting);
		}
	}
	else if (function == ADNumImages)
	{
		det->setFrameCount(value);
	}
	else if (function == ADTriggerMode)
	{		
		if      (value == 0)    { det->setTriggerMode(xsp::lambda::TrigMode::SOFTWARE); }
		else if (value == 1)    { det->setTriggerMode(xsp::lambda::TrigMode::EXT_SEQUENCE); }
		else if (value == 2)    { det->setTriggerMode(xsp::lambda::TrigMode::EXT_FRAMES); }
	}
	else if ((function == ADSizeX) || (function == ADSizeY) ||
	         (function == ADMinX)  || (function == ADMinY) )
	{
		status |= setIntegerParam(addr, ADMinX, 0);
		status |= setIntegerParam(addr, ADMinY, 0);
		status |= setIntegerParam(addr, ADSizeX, recs[addr]->frameWidth());
		status |= setIntegerParam(addr, ADSizeY, recs[addr]->frameHeight());
		callParamCallbacks();
	}
	else if (function == LAMBDA_OperatingMode) 
	{
		if (value == 0)
		{
			det->setOperationMode(xsp::lambda::OperationMode(xsp::lambda::BitDepth::DEPTH_1));
			setIntegerParam(NDDataType, NDUInt8);
		}
		else if (value == 1)
		{
			det->setOperationMode(xsp::lambda::OperationMode(xsp::lambda::BitDepth::DEPTH_6));
			setIntegerParam(NDDataType, NDUInt8);
		}
		else if (value == 2)
		{
			det->setOperationMode(xsp::lambda::OperationMode(xsp::lambda::BitDepth::DEPTH_12));
			setIntegerParam(NDDataType, NDUInt16);
		}
		else if(value == 3) 
		{
			det->setOperationMode(xsp::lambda::OperationMode(xsp::lambda::BitDepth::DEPTH_24));
			setIntegerParam(NDDataType, NDUInt32);
		}
		else if (value == 4)
		{
			det->setOperationMode(xsp::lambda::OperationMode(xsp::lambda::BitDepth::DEPTH_1,
			                                                 xsp::lambda::ChargeSumming::OFF,
			                                                 xsp::lambda::CounterMode::DUAL));
			setIntegerParam(NDDataType, NDUInt8);
		}
		else if (value == 5)
		{
			det->setOperationMode(xsp::lambda::OperationMode(xsp::lambda::BitDepth::DEPTH_6,
			                                                 xsp::lambda::ChargeSumming::OFF,
			                                                 xsp::lambda::CounterMode::DUAL));
			setIntegerParam(NDDataType, NDUInt8);
		}
		else if (value == 6)
		{
			det->setOperationMode(xsp::lambda::OperationMode(xsp::lambda::BitDepth::DEPTH_12, 
			                                                 xsp::lambda::ChargeSumming::OFF,
			                                                 xsp::lambda::CounterMode::DUAL));
			setIntegerParam(NDDataType, NDUInt16);

		}
		else if (value == 7)
		{
			det->setOperationMode(xsp::lambda::OperationMode(xsp::lambda::BitDepth::DEPTH_24, 
			                                                 xsp::lambda::ChargeSumming::OFF,
			                                                 xsp::lambda::CounterMode::DUAL));
			setIntegerParam(NDDataType, NDUInt32);
			
		}
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
static const iocshArg LambdaConfigArg3 = { "Readout Threads", iocshArgInt };
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
