/**
 Copyright (c) 2015, UChicago Argonne, LLC
 See LICENSE file.
 */
/* ADLambda.cpp */
#include <iocsh.h>

#include <epicsTime.h>
#include <epicsExit.h>
#include <epicsExport.h>


#include "ADLambda.h"
#include <LambdaSysImpl.h>

static void lambdaHandleNewImageTaskC(void *drvPvt);
//static void lambdaHandleNewImageTaskMultiC(void *drvPvt);

extern "C" {
/** Configuration command for PICAM driver; creates a new PICam object.
 * \param[in] portName The name of the asyn port driver to be created.
 * \param[in] path to the config files.
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
int LambdaConfig(const char *portName, const char* configPath, int maxBuffers,
        size_t maxMemory, int priority, int stackSize) {
    new ADLambda(portName, configPath, maxBuffers, maxMemory, priority, stackSize);
    return (asynSuccess);
}

/**
 * Callback function for exit hook
 */
static void exitCallbackC(void *pPvt) {
    ADLambda *pADLambda = (ADLambda*) pPvt;
    delete pADLambda;
}

}

const char *ADLambda::driverName = "Lambda";

/**
 * Constructor
 * \param[in] portName The name of the asyn port driver to be created.
 * \param[in] directory containing configuration file location
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
ADLambda::ADLambda(const char *portName, const char *configPath, int maxBuffers,
        size_t maxMemory, int priority, int stackSize) :
        ADDriver(portName, 1, int(NUM_LAMBDA_PARAMS), maxBuffers, maxMemory,
                asynEnumMask, asynEnumMask, ASYN_CANBLOCK, 1, priority,
                stackSize) {

    int status = asynSuccess;
    acquiringData = false;
    frameNumbersRead = 0;
    totalLossFramesRead = 0;
    latestImageNumberRead = 0;
    imageThreadKeepAlive = false;

    lambdaInstance = new DetCommonNS::LambdaSysImpl(configPath);
    printf("Done making instance");
    status |= ADDriver::createParam(LAMBDA_EnergyThresholdString,
            asynParamFloat64, &LAMBDA_EnergyThreshold);
    status |= ADDriver::createParam(LAMBDA_DecodedQueueDepthString,
            asynParamInt32, &LAMBDA_DecodedQueueDepth);
    status |= ADDriver::createParam(LAMBDA_BadFrameCounterString,
            asynParamInt32, &LAMBDA_BadFrameCounter);
    status |= ADDriver::createParam(LAMBDA_BadImageString,
            asynParamInt32, &LAMBDA_BadImage);
    initializeDetector();

    /* create the thread that updates new images */
    status = (epicsThreadCreate("lambdaHandleNewImageTaskC",
            epicsThreadPriorityMedium,
            epicsThreadGetStackSize(epicsThreadStackMedium),
            (EPICSTHREADFUNC)lambdaHandleNewImageTaskC,
            this) == NULL);

    epicsAtExit(exitCallbackC, this);
}

/**
 * Destructor
 */
ADLambda::~ADLambda() {
    imageThreadKeepAlive = false;
}

/**
 * Method called by WriteInt32 intecepts message to start
 * acquisition.
 */
asynStatus ADLambda::acquireStart(){
    int sizeX, sizeY, imageDepth;
    asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
            "%s:%s Enter\n", driverName, __FUNCTION__);
    int status = asynSuccess;
    
    setIntegerParam(ADNumImagesCounter, 0);
    setIntegerParam(LAMBDA_BadFrameCounter, 0);
    callParamCallbacks();
    lambdaInstance->GetImageFormat(sizeX, sizeY, imageDepth);
    imageDims[0] = sizeX;
    imageDims[1] = sizeY;
    imageDataType = NDUInt16;
    acquiringData = true;
    acquiredImages = 0;
    lambdaInstance->StartImaging();

    asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
            "%s:%s Exit\n", driverName, __FUNCTION__);
    return (asynStatus)status;
}

/**
 * Method called when WriteInt32 intercepts a message to stop
 * acquisition or if handleNewImageTask finds the requested
 * number of images have been captured.
 */
asynStatus ADLambda::acquireStop(){
    asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
            "%s:%s Enter\n", driverName, __FUNCTION__);

    int status = asynSuccess;

    lambdaInstance->StopImaging();

    setIntegerParam(ADStatus, ADStatusIdle);
    setIntegerParam(ADAcquire, 0);
    callParamCallbacks();

    asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
            "%s:%s Exit\n", driverName, __FUNCTION__);
    return (asynStatus)status;
}

/**
 * Pass through method to the vendor object requesting an image.
 * Returned image will be returned as a pointer to int.
 *  \param[in] The requested frame number in the queue.
 *  \param[out] Error code for returned image.
 *  \return A pointer to integer array representing the requested image.
 */
int* ADLambda::getDecodedImageInt(long& lFrameNo, short& shErrCode){
    return lambdaInstance->GetDecodedImageInt(lFrameNo, shErrCode);
}

/**
 * Pass through method to the vendor object requesting an image.
 * Returned image will be returned as a pointer to short.
 *  \param[in] The requested frame number in the queue.
 *  \param[out] Error code for returned image.
 *  \return A pointer to short array representing the requested image.
 */
short* ADLambda::getDecodedImageShort(long& lFrameNo, short& shErrCode){
    return lambdaInstance->GetDecodedImageShort(lFrameNo, shErrCode);
}

int ADLambda::getImageDepth(){
    return 12;
}

/**
 * Pass through method to get the depth of the raw Image
 * queue.
 *  \return Image queue depth
 */
int ADLambda::getQueueDepth(){
    return lambdaInstance->GetQueueDepth();
}

/**
 * Pass through method to the vendor object to get format information
 * for images
 *  \param[out] nX number of pixels in X dimension
 *  \param[out] nY number of pixels in Y dimension
 *  \param[out] number of bits of image Depth
 */
void ADLambda::getImageFormat(int& nX, int& nY, int& nImgDepth) {
    lambdaInstance->GetImageFormat( (int &)nX, (int &)nY, (int &)nImgDepth);
}

/**
 * This method is called by handleNewImageTaskC as the image handling thread
 * is created.  This method runs a continuous loop which constantly checks
 * for images in the buffer queue.  When new images are seen they are pulled
 * off the queue and placed into an NDArray for use by areaDetector
 */
void ADLambda::handleNewImageTask() {
    long numBufferedImages;
    short *shDecodedData;
    int *decodedData;
    long currentFrameNumber;
    short frameErrorCode;
    bool bRead;
    bool firstFrame;
    //long acquiredImages;
    long startFrame;
    long lossFrames;
    long currentFrameNo;
    int imageCounter;
    int arrayCounter;
    int arrayCallbacks;
    int numBadFrames;
    NDArrayInfo arrayInfo;
    epicsTimeStamp currentTime;

    imageThreadKeepAlive = true;
    startFrame = 0;
    bRead = false;
    acquiredImages = 0;
    lossFrames = 0;
    currentFrameNo = 0;
    currentFrameNumber = -1;

    while (true && imageThreadKeepAlive) {
        //epicsThreadSleep(0.000025);
        numBufferedImages = lambdaInstance->GetQueueDepth();
        getIntegerParam(ADNumImages, &frameNumbersRead);

        if (numBufferedImages > 0 ) {
            setIntegerParam(LAMBDA_DecodedQueueDepth, (int)numBufferedImages);
            callParamCallbacks();
            asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
                    "%s:%s numberBufferedImages: %d\n",
                    driverName, __FUNCTION__,
                    (int) numBufferedImages);
            if (getImageDepth() == 12){
                long newFrameNumber;
                shDecodedData = getDecodedImageShort(newFrameNumber, frameErrorCode);
                if (newFrameNumber == currentFrameNumber && newFrameNumber != -1) {
                    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                        "%s:%s shDecodedData %p, Current Frame Number %ld,"
                            " frameErrorCode %d equals last image\n",
                        driverName, __FUNCTION__,
                        shDecodedData,
                        currentFrameNumber,
                        frameErrorCode);
                }
                if ((newFrameNumber !=-1) && (currentFrameNumber != -1)
                        && (newFrameNumber - currentFrameNumber) != 1) {
                    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                        "%s:%s  missing %ld Frames starting at  Frame Number %ld\n",
                        driverName, __FUNCTION__,
                        currentFrameNumber + 1,
                        newFrameNumber - currentFrameNumber);
                }
                currentFrameNumber = newFrameNumber;
                if ((shDecodedData == NULL) &&
                        (currentFrameNumber == -1) &&
                        (frameErrorCode == -1)) {
                    bRead = false;
                }
                else {
                    bRead = true;
                }
                if (bRead && acquiredImages < frameNumbersRead){
                    asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
                            "%s:%s Processing Image 12 bit\n",
                                driverName, __FUNCTION__);
                    //Pass along images
                    getIntegerParam(ADNumImagesCounter, &imageCounter);
                    imageCounter++;
                    setIntegerParam(ADNumImagesCounter, imageCounter);
                    /* Update the image */
                    /* First clear the image */
                    if (this->pArrays[0]) {
                        this->pArrays[0]->release();
                    }
                    getIntegerParam(NDArrayCallbacks, &arrayCallbacks);
                    if (arrayCallbacks){
                        asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
                                "%s:%s Callbacks enabled\n",
                                driverName, __FUNCTION__);
                        /* Allocate an new array */
                        this->pArrays[0] = pNDArrayPool->alloc(2, imageDims,
                                imageDataType, 0,
                                NULL);
                        if (this->pArrays[0] != NULL){
                            asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
                                    "%s:%s Arrays not Null\n",
                                    driverName, __FUNCTION__);
                            pImage = this->pArrays[0];
                            pImage->getInfo(&arrayInfo);
                            //copy the data from the input to the output
                            memcpy(pImage->pData, shDecodedData,
                                    arrayInfo.totalBytes);
                            getIntegerParam(NDArrayCounter, &arrayCounter);
                            arrayCounter++;
                            setIntegerParam(NDArrayCounter, arrayCounter);
                            setIntegerParam(NDArraySize, arrayInfo.totalBytes);
                            /* Timestamps */
                            epicsTimeGetCurrent(&currentTime);
                            pImage->timeStamp = currentTime.secPastEpoch +
                                    currentTime.nsec / 1.e9;
                            updateTimeStamp(&pImage->epicsTS);
                            pImage->uniqueId = currentFrameNumber;
                            if (frameErrorCode != 0){
                                asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
                                        "%s:%s Bad Frame\n",
                                        driverName, __FUNCTION__);
                                getIntegerParam(LAMBDA_BadFrameCounter, &numBadFrames);
                                numBadFrames++;
                                setIntegerParam(LAMBDA_BadFrameCounter, numBadFrames);
                                setIntegerParam(LAMBDA_BadImage, 1);
                            }
                            else {
                                setIntegerParam(LAMBDA_BadImage, 0);
                            }
                            callParamCallbacks();
                            /* Get attributes that have been defined for this driver */
                            getAttributes(pImage->pAttributeList);
                            doCallbacksGenericPointer(pImage, NDArrayData, 0);
                        }

                    }

                }

            }
            else if (getImageDepth() == 24){
                decodedData = getDecodedImageInt(currentFrameNumber, frameErrorCode);
                if ((decodedData == NULL) &&
                        (currentFrameNumber == -1) &&
                        (frameErrorCode == -1)) {
                    bRead = false;
                }
                else {
                    bRead = true;
                }
                if (bRead && acquiredImages < frameNumbersRead){
                    asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
                            "%s:%s Processing Image 24 bit",
                            driverName, __FUNCTION__);
                    //Pass Along images
                }
            }
            if (bRead && acquiredImages < frameNumbersRead){
                if (firstFrame) {
                    startFrame = currentFrameNo;
                    firstFrame = false;
                }
                currentFrameNo = currentFrameNo - startFrame + 1;
                /* ERROR CODE from 0-7
                 * 0: image is OK
                 */
                if (frameErrorCode != 0) {
                    lossFrames++;
                    totalLossFramesRead = lossFrames;
                }
                latestImageNumberRead = currentFrameNo;
                acquiredImages++;
            }
        }
        else {
            if (acquiringData == true && acquiredImages >= frameNumbersRead){
                acquiringData = false;
                acquiredImages=0;
                acquireStop();
            }
        }
    }
}


/**
 * This method is called by the constructor to initialize areaDetector
 * parameters.
 */
asynStatus ADLambda::initializeDetector(){
    int status = asynSuccess;
    lambdaInstance->GetImageFormat(imageWidth, imageHeight, imageDepth);
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s imageHeight %d, imageWidth%d\n",
            driverName, __FUNCTION__,
            imageHeight, imageWidth);
    setIntegerParam(ADMaxSizeX, imageWidth);
    setIntegerParam(ADMaxSizeY, imageHeight);
    setIntegerParam(ADMinX, 0);
    setIntegerParam(ADMinY, 0);
    setIntegerParam(ADSizeX, imageWidth);
    setIntegerParam(ADSizeY, imageHeight);
    setIntegerParam(NDArraySizeX, imageWidth);
    setIntegerParam(NDArraySizeY, imageHeight);
    setIntegerParam(NDArraySize, 0);

    callParamCallbacks();


    return (asynStatus)status;
}

/**
 * Override super class's report method to provide detector specific info.
 * When done, call ADDriver (direct super class) method to provide info from
 * the upper classes.
 */
void ADLambda::report(FILE *fp, int details) {
    ADDriver::report(fp, details);
}

/**
 * Override from super class to handle detector specific parameters.
 * If the parameter is from one of the super classes and is not handled
 * here, then pass along to ADDriver (direct super class)
 */
asynStatus ADLambda::writeFloat64(asynUser *pasynUser, epicsFloat64 value) {
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
        lambdaInstance->SetShutterTime(shutterTime);
    }
    else if (function == ADAcquirePeriod){
        printf("Setting Acquire Period\n");
        acquirePeriod = value;
        lambdaInstance->SetDelayTime(acquirePeriod);
        setDoubleParam(function, acquirePeriod);
    }
    else if (function == LAMBDA_EnergyThreshold){
        lambdaInstance->SetThreshold(0, value);
    }
    else {
        if (function < LAMBDA_FIRST_PARAM) {
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
    int function = pasynUser->reason;
    int adStatus;
    int adacquiring;

    //Record for later use
    getIntegerParam(ADStatus, &adStatus);
    getIntegerParam(ADAcquire, &adacquiring);

    /** Make sure that we write the value to the param
     *
     */
    setIntegerParam(function, value);

    if (function == ADAcquire){
        if (value && !adacquiring) {
            acquireStart();
        }
        else if (!value && adacquiring){
            acquireStop();
        }
    }
    else if (function == ADNumImages){
        lambdaInstance->SetNImages(value);
    }
    else if (function == ADTriggerMode){
        lambdaInstance->SetTriggerMode(value);
    }
    else if (function < LAMBDA_FIRST_PARAM) {
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
asynStatus ADLambda::writeOctet(asynUser* pasynUser, const char *value, size_t nChars, size_t *nActual){
    int status = asynSuccess;
    int function = pasynUser->reason;

    if (function < LAMBDA_FIRST_PARAM) {
        status = ADDriver::writeOctet(pasynUser, value, nChars, nActual);
    }
    return (asynStatus) status;
}
/**
 * Thread Handler function for receiving new images.
 */
static void lambdaHandleNewImageTaskC(void *drvPvt)
{
    ADLambda *pPvt = (ADLambda *)drvPvt;

    pPvt->handleNewImageTask();
}


/* Code for iocsh registration */

/* PICamConfig */
static const iocshArg LambdaConfigArg0 = { "Port name", iocshArgString };
static const iocshArg LambdaConfigArg1 = { "Config file path", iocshArgString };
static const iocshArg LambdaConfigArg2 = { "maxBuffers", iocshArgInt };
static const iocshArg LambdaConfigArg3 = { "maxMemory", iocshArgInt };
static const iocshArg LambdaConfigArg4 = { "priority", iocshArgInt };
static const iocshArg LambdaConfigArg5 = { "stackSize", iocshArgInt };
static const iocshArg * const LambdaConfigArgs[] = { &LambdaConfigArg0,
        &LambdaConfigArg1, &LambdaConfigArg2, &LambdaConfigArg3,
        &LambdaConfigArg4, &LambdaConfigArg5 };

static void configLambdaCallFunc(const iocshArgBuf *args) {
    LambdaConfig(args[0].sval, args[1].sval, args[2].ival, args[3].ival,
            args[4].ival, args[5].ival);
}
static const iocshFuncDef configLambda = { "LambdaConfig", 6, LambdaConfigArgs };

static void LambdaRegister(void) {
    iocshRegister(&configLambda, configLambdaCallFunc);
}

extern "C" {
epicsExportRegistrar(LambdaRegister);
}
