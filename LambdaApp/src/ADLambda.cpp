/**
 Copyright (c) 2015, UChicago Argonne, LLC
 See LICENSE file.
 */
/* ADLambda.cpp */
#include <algorithm>
#include <iocsh.h>
#include <vector>
#include <iostream>

#include <epicsTime.h>
#include <epicsExit.h>
#include <epicsExport.h>


#include "ADLambda.h"
#include <libxsp.h>

typedef struct
{
	ADLambda* driver;
	int receiver;
} acquire_data;

static void acquire_thread_callback(void *drvPvt)    { ((ADLambda*) drvPvt)->waitAcquireThread(); }
static void monitor_thread_callback(void *drvPvt)    { ((ADLambda*) drvPvt)->monitorThread(); }

static void receiver_acquire_callback(void *drvPvt)
{
	acquire_data* data = (acquire_data*) drvPvt;
	
	data->driver->acquireThread(data->receiver);
}

extern "C" 
{
	/** Configuration command for Lambda driver; creates a new ADLambda object.
	 * \param[in] portName The name of the asyn port driver to be created.
	 * \param[in] configPath to the config files.
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
	 */
	void LambdaConfig(const char *portName, const char* configPath, int numModules) 
	{
		new ADLambda(portName, configPath, numModules);
	}

}

const char *ADLambda::driverName = "Lambda";
const int ADLambda::TWELVE_BIT = 12;
const int ADLambda::TWENTY_FOUR_BIT = 24;

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
ADLambda::ADLambda(const char *portName, const char *configPath, int numModules) :
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
	createParam(LAMBDA_VersionNumberString, asynParamOctet, &LAMBDA_VersionNumber);
	createParam(LAMBDA_ConfigFilePathString, asynParamOctet, &LAMBDA_ConfigFilePath);
	createParam(LAMBDA_EnergyThresholdString, asynParamFloat64, &LAMBDA_EnergyThreshold);
	createParam(LAMBDA_DecodedQueueDepthString, asynParamInt32, &LAMBDA_DecodedQueueDepth);
	createParam(LAMBDA_OperatingModeString, asynParamInt32, &LAMBDA_OperatingMode);
	createParam(LAMBDA_DetectorStateString, asynParamInt32, &LAMBDA_DetectorState);
	createParam(LAMBDA_BadFrameCounterString, asynParamInt32, &LAMBDA_BadFrameCounter);
	createParam(LAMBDA_MedipixIDsString, asynParamOctet, &LAMBDA_MedipixIDs);
	createParam(LAMBDA_DetCoreVersionNumberString, asynParamOctet, &LAMBDA_DetCoreVersionNumber);
	createParam(LAMBDA_BadImageString, asynParamInt32, &LAMBDA_BadImage);

	this->startAcquireEvent = new epicsEvent();
	this->threadFinishEvent = new epicsEvent();
	
	this->connect();
}

ADLambda::~ADLambda()    { this->disconnect(); }

asynStatus ADLambda::connect()
{
	sys = xsp::createSystem(configFileName);
	
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
		
		if (rec->frameDepth() == TWELVE_BIT) 
		{
			setIntegerParam(index, NDDataType, NDUInt16);
			setIntegerParam(index, LAMBDA_OperatingMode, 0);
		}
		else if (rec->frameDepth() == TWENTY_FOUR_BIT) 
		{
			setIntegerParam(index, NDDataType, NDUInt32);
			setIntegerParam(index, LAMBDA_OperatingMode, 1);
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
	asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s Enter\n", driverName, __FUNCTION__);

	int status = asynSuccess;

	det->stopAcquisition();

	setIntegerParam(ADStatus, ADStatusIdle);
	setIntegerParam(ADAcquire, 0);
	callParamCallbacks();

	asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s Exit\n", driverName, __FUNCTION__);
	return (asynStatus)status;
}

void ADLambda::processTwelveBit(const void* data, NDArrayInfo arrayInfo)
{
	const uint16_t* frame_data = reinterpret_cast<const uint16_t*>(data);	

	switch (imageDataType) 
	{
		case NDUInt8:
		{
			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "Copying from UInt16 to UInt8 %d\n", imageDataType);
			
			int scaleBytes = sizeof(epicsUInt16)/sizeof(epicsUInt8);
			const uint16_t*first = frame_data;
			const uint16_t*last = frame_data + arrayInfo.totalBytes/scaleBytes;
			unsigned char  *result = (unsigned char *)pImage->pData;
			
			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "Copying from UInt16 to UInt8 %d %p, %p , %p\n", 
				      imageDataType, first, last, result);
			
			std::copy(first, last, result);
		}
		break;
		
		case NDInt16:
		{
			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "Copying from UInt16 to Int16 %d\n", imageDataType);
				   
			int scaleBytes = sizeof(epicsUInt16)/sizeof(epicsInt16);
			const uint16_t*first = frame_data;
			const uint16_t*last = frame_data + arrayInfo.totalBytes/scaleBytes;
			short  *result = (short *)pImage->pData;

			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "Copying from UInt16 to Int16 %d %p, %p , %p\n",
				      imageDataType, first, last, result);
				      
			std::copy(first, last, result);
		}
		break;
		
		case NDUInt16:
		{
			memcpy(pImage->pData, frame_data, arrayInfo.totalBytes);
		}
		break;
		
		case NDInt32:
		{
			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "Copying from UInt16 to Int32 %d\n", imageDataType);
			
			int scaleBytes = sizeof(epicsUInt16)/sizeof(epicsInt32);
			const uint16_t*first = frame_data;
			const uint16_t*last = frame_data + arrayInfo.totalBytes/scaleBytes;
			int  *result = (int *)pImage->pData;
			
			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "Copying from UInt16 to Int32 %d %p, %p , %p\n",
				      imageDataType, first, last, result);
				      
			std::copy(first, last, result);
		}
		break;
		
		case NDUInt32:
		{
			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "Copying from UInt16 to UInt32 %d\n", imageDataType);
			
			int scaleBytes = sizeof(epicsUInt16)/sizeof(epicsUInt32);
			const uint16_t*first = frame_data;
			const uint16_t*last = frame_data + arrayInfo.totalBytes/scaleBytes;
			unsigned int  *result = (unsigned int *)pImage->pData;
			
			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "Copying from UInt16 to UInt32 %d %p, %p , %p\n",
				      imageDataType, first, last, result);
				      
			std::copy(first, last, result);
		}
		break;
		
		case NDFloat32:
		case NDFloat64:
		default:
		{
			asynPrint (pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s Unhandled Data Type %d", 
				       driverName, __FUNCTION__, imageDataType);
		}
		break;
	}
}

void ADLambda::processTwentyFourBit(const void* data, NDArrayInfo arrayInfo)
{
	const uint32_t* frame_data = reinterpret_cast<const uint32_t*>(data);

	switch (imageDataType) 
	{
		case NDUInt8:
		{
			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "Copying from UInt32 to UInt8 %d\n", imageDataType);
			
			int scaleBytes = sizeof(epicsUInt32)/sizeof(epicsUInt8);
			const uint32_t* first = frame_data;
			const uint32_t* last = frame_data + arrayInfo.totalBytes/scaleBytes;
			unsigned char  *result = (unsigned char *)pImage->pData;
			
			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "Copying from UInt32 to UInt8 %d %p, %p , %p\n",
			          imageDataType, first, last, result);
			          
			std::copy(first, last, result);
		}
		break;
		
		
		case NDInt16:
		{
			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "Copying from UInt32 to Int16 %d\n", imageDataType);
			
			int scaleBytes = sizeof(epicsUInt32)/sizeof(epicsInt16);
			const uint32_t* first = frame_data;
			const uint32_t* last = frame_data + arrayInfo.totalBytes/scaleBytes;
			short  *result = (short *)pImage->pData;
			
			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "Copying from UInt32 to Int16 %d %p, %p , %p\n",
			          imageDataType, first, last, result);
			          
			std::copy(first, last, result);
		}
		break;
		
		
		case NDUInt16:
		{
			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "Copying from UInt32 to UInt16 %d\n", imageDataType);
			
			int scaleBytes = sizeof(epicsUInt32)/sizeof(epicsUInt16);
			const uint32_t* first = frame_data;
			const uint32_t* last = frame_data + arrayInfo.totalBytes/scaleBytes;
			unsigned short  *result = (unsigned short *)pImage->pData;
			
			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "Copying from UInt32 to UInt16 %d %p, %p , %p\n",
			          imageDataType, first, last, result);
			          
			std::copy(first, last, result);
		}
		break;
		
		case NDInt32:
		{
			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "Copying from UInt32 to Int32 %d\n", imageDataType);
			
			int scaleBytes = sizeof(epicsUInt32)/sizeof(epicsUInt32);
			const uint32_t* first = frame_data;
			const uint32_t* last = frame_data + arrayInfo.totalBytes/scaleBytes;
			int  *result = (int *)pImage->pData;
			
			asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, "Copying from UInt32 to Int32 %d %p, %p , %p\n",
			          imageDataType, first, last, result);
			          
			std::copy(first, last, result);
		}
		break;
		
		case NDUInt32:
		{
			memcpy(pImage->pData, frame_data, arrayInfo.totalBytes);
		}
		break;
		
		case NDFloat32:
		case NDFloat64:
		default:
		{
			asynPrint (pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s Unhandled Data Type %d\n",
			           driverName, __FUNCTION__, imageDataType);
		}
		break;

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
			bool signal = this->startAcquireEvent->wait(.000025);
		this->lock();
		
		if (!signal)    { continue; }
		
		int datatype;
		this->getIntegerParam(NDDataType, &datatype);
	
		this->imageDataType = datatype;
		
		det->startAcquisition();
		
		for (int index = 0; index < this->recs.size(); index += 1)
		{
			acquire_data data;
			
			data.driver = this;
			data.receiver = index;
		
			epicsThreadCreate("ADLambda::acquireThread()",
	                  epicsThreadPriorityLow,
	                  epicsThreadGetStackSize(epicsThreadStackMedium),
	                  (EPICSTHREADFUNC)::receiver_acquire_callback,
	                  &data);
		}
		
		int threads_running = this->recs.size();
		
		while(threads_running != 0)
		{
			this->threadFinishEvent->wait();
			
			threads_running -= 1;
		}
		
		acquireStop();
	}
	
	this->unlock();
}

void ADLambda::monitorThread()
{
}
		
void ADLambda::acquireThread(int receiver)
{
	double ONE_BILLION = 1.E9;

	int numAcquired = 0;
    setIntegerParam(receiver, ADNumImagesCounter, 0);
	setIntegerParam(receiver, LAMBDA_BadFrameCounter, 0);
	callParamCallbacks();

	size_t imagedims[2];
	
	int width, height;
	
	this->getIntegerParam(ADSizeX, &width);
	this->getIntegerParam(ADSizeY, &height);
	
	imagedims[0] = width;
	imagedims[1] = height;

	/*
	 * Each module will use address 0's number of images
	 * so that the user doesn't have to change the number
	 * of images for each module separately.
	 */
	int toRead;
	getIntegerParam(ADNumImages, &toRead);
	
	while (numAcquired < toRead &&
	       (this->recs[receiver]->isBusy() || this->recs[receiver]->framesQueued()))
	{	
		int numBuffered = this->recs[receiver]->framesQueued();
		
		if (numBuffered == 0)    { continue; }
		
		this->setIntegerParam(receiver, LAMBDA_DecodedQueueDepth, numBuffered);
		
		const xsp::Frame* frame = this->recs[receiver]->frame(1500);

		long frameNo = frame->nr();
		const void* data = frame->data();
		
		numAcquired += 1;
		this->setIntegerParam(receiver, ADNumImagesCounter, numAcquired);
		this->callParamCallbacks();
		
		if (data == NULL || frame->status() != xsp::FrameStatusCode::FRAME_OK)
		{
			int badFrames;
			
			this->getIntegerParam(receiver, LAMBDA_BadFrameCounter, &badFrames);
			badFrames++;
			this->setIntegerParam(receiver, LAMBDA_BadFrameCounter, badFrames);
			this->setIntegerParam(LAMBDA_BadImage, 1);
			this->callParamCallbacks();
			
			this->recs[receiver]->release(frameNo);
			
			continue;
		}
		else
		{
			this->setIntegerParam(receiver, LAMBDA_BadImage, 0);
			this->callParamCallbacks();
		}

		if (this->pArrays[0])    { this->pArrays[0]->release(); }
		
		int arrayCallbacks;
		getIntegerParam(NDArrayCallbacks, &arrayCallbacks);
		
		if (arrayCallbacks)
		{
			this->pArrays[0] = pNDArrayPool->alloc(2, imagedims, this->imageDataType, 0, NULL);
		
			pImage = this->pArrays[0];
			
			NDArrayInfo arrayInfo;
			pImage->getInfo(&arrayInfo);
			
			int bitDepth = this->recs[receiver]->frameDepth();
			
			if      (bitDepth == TWELVE_BIT)         { this->processTwelveBit(data, arrayInfo); }
			else if (bitDepth == TWENTY_FOUR_BIT)    { this->processTwentyFourBit(data, arrayInfo); }
			
			int arrayCounter;
			getIntegerParam(NDArrayCounter, &arrayCounter);
			arrayCounter += 1;
			setIntegerParam(NDArrayCounter, arrayCounter);
			
			setIntegerParam(NDArraySize, arrayInfo.totalBytes);
			
			/* Time stamps */
			epicsTimeStamp currentTime;
			epicsTimeGetCurrent(&currentTime);
			pImage->timeStamp = currentTime.secPastEpoch + currentTime.nsec / ONE_BILLION;
			updateTimeStamp(&pImage->epicsTS);
			
			pImage->uniqueId = frameNo;
			
			callParamCallbacks();
			
			/* get attributes that have been defined for this
			* driver */
			getAttributes(pImage->pAttributeList);
			doCallbacksGenericPointer(pImage, NDArrayData, receiver);
		}
		
		this->recs[receiver]->release(frameNo);
	}
	
	this->threadFinishEvent->trigger();
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
	double shutterTime;
	double acquirePeriod;

	setDoubleParam(function, value);
	asynPrint(pasynUser, ASYN_TRACE_ERROR,
	        "Entering %s:%s\n",
	        driverName,
	        __func__ );
	if (function == ADAcquireTime) {
		shutterTime = value * 1000.0;
		det->setShutterTime(shutterTime);
	}
	else if (function == ADAcquirePeriod)
	{
		printf("Setting Acquire Period\n");
		acquirePeriod = value;
		setDoubleParam(function, acquirePeriod);
	}
	else if (function == LAMBDA_EnergyThreshold)
	{
		det->setThresholds(std::vector<double>{value});
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

/**
 * Override from super class to handle detector specific parameters.
 * If the parameter is from one of the super classes and is not handled
 * here, then pass along to ADDriver (direct super class)
 */
asynStatus ADLambda::writeInt32(asynUser *pasynUser, epicsInt32 value) {
	int status = asynSuccess;

	int addr;
	int function;
	const char* paramName;

	status = parseAsynUser(pasynUser, &function, &addr, &paramName);
	if (status != asynSuccess)    { return status; }

	//Record for later use
	int adStatus;
	int adacquiring;
	getIntegerParam(ADStatus, &adStatus);
	getIntegerParam(ADAcquire, &adacquiring);

	/** Make sure that we write the value to the param
	 *
	 */
	setIntegerParam(addr, function, value);

	if (function == ADAcquire)
	{
		if (value && !adacquiring)         { this->startAcquireEvent->trigger(); }
		else if (!value && adacquiring)    { acquireStop(); }
	}
	else if (function == ADNumImages){
		det->setFrameCount(value);
	}
	else if (function == ADTriggerMode)
	{
		asynPrint(pasynUser, ASYN_TRACE_ERROR,
		        "%s:%s Setting TriggerMode %d\n",
		        driverName,
		        __FUNCTION__,
		        value);
		
		if      (value == 0)    { det->setTriggerMode(xsp::lambda::TrigMode::SOFTWARE); }
		else if (value == 1)    { det->setTriggerMode(xsp::lambda::TrigMode::EXT_SEQUENCE); }
		else if (value == 2)    { det->setTriggerMode(xsp::lambda::TrigMode::EXT_FRAMES); }
	}
	else if ((function == ADSizeX) ||
	         (function == ADSizeY) ||
	         (function == ADMinX) ||
	         (function == ADMinY) )
	{
		status |= setIntegerParam(addr, ADMinX, 0);
		status |= setIntegerParam(addr, ADMinY, 0);
		status |= setIntegerParam(addr, ADSizeX, recs[addr]->frameWidth());
		status |= setIntegerParam(addr, ADSizeY, recs[addr]->frameHeight());
		callParamCallbacks();
	}
	else if (function == LAMBDA_OperatingMode) 
	{
		asynPrint(pasynUser, ASYN_TRACE_ERROR,
		        "%s:%s Setting TriggerMode %d\n",
		        driverName,
		        __FUNCTION__,
		        value);
		if (value == 0)
		{
			det->setOperationMode(xsp::lambda::OperationMode(xsp::lambda::BitDepth::DEPTH_12));
			setIntegerParam(NDDataType, NDUInt16);
			callParamCallbacks();
		}
		else if(value == 1) 
		{
			det->setOperationMode(xsp::lambda::OperationMode(xsp::lambda::BitDepth::DEPTH_24));
			setIntegerParam(NDDataType, NDUInt32);
			callParamCallbacks();
		}
	}
	else if (function < LAMBDA_FIRST_PARAM) 
	{
		status = ADDriver::writeInt32(pasynUser, value);
	}

	callParamCallbacks();
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
