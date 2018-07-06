/**
 Copyright (c) 2015, UChicago Argonne, LLC
 See LICENSE file.
 */
/* ADLambda.h
 *
 * This is an areaDetector driver for cameras that communicate
 * with the X-Spectrum Lambda driver library
 *
 */
#ifndef ADLAMBDA_H
#define ADLAMBDA_H

namespace DetLambdaNS {
class LambdaSysImpl;
class LambdaInterface;}
using namespace DetLambdaNS;

#include <epicsString.h>
#include <epicsEvent.h>
#include <epicsThread.h>

#include <LambdaSysImpl.h>

#include "ADDriver.h"

/**
 * Class to wrap Lambda detector library provided by X-Spectrum
 */
class epicsShareClass ADLambda: public ADDriver {
public:
    static const char *driverName;
    static const int TWELVE_BIT, TWENTY_FOUR_BIT;
    ADLambda(const char *portName, const char *configPath, int maxBuffers,
            size_t maxMemory, int priority, int stackSize);
    ~ADLambda();
    virtual asynStatus disconnect(asynUser* paasynUser);
    virtual asynStatus connect(asynUser* pasynUser);
    asynStatus createImageHandlerThread();
    void killImageHandlerThread();
    int getImageDepth();
//    int getQueueDepth();
    short* getDecodedImageShort(int32& lFrameNo, int16& shErrCode);
    int* getDecodedImageInt(int32& lFrameNo, int16& shErrCode);
//    void getImageFormat(int& nX, int& nY, int& nImgDepth);
    void handleNewImageTask(void);
    void report(FILE *fp, int details);
    virtual asynStatus  readInt32 (asynUser *pasynUser, epicsInt32 *value);
    virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    virtual asynStatus writeOctet(asynUser *pasynUser, const char *value,
        size_t nChars, size_t *nActual);
    virtual asynStatus 	readOctet (asynUser *pasynUser, char *value, 
        size_t maxChars, size_t *nActual, int *eomReason);

protected:
    // int reference to parameters
    int LAMBDA_VersionNumber;
#define LAMBDA_FIRST_PARAM LAMBDA_VersionNumber

    int LAMBDA_ConfigFilePath;
    int LAMBDA_EnergyThreshold;
    int LAMBDA_DecodedQueueDepth;
    int LAMBDA_OperatingMode;
    int LAMBDA_DetectorState;
    int LAMBDA_BadFrameCounter;
    int LAMBDA_BadImage;
#define LAMBDA_LAST_PARAM LAMBDA_BadImage

private:
    bool imageThreadKeepAlive;
    asynStatus acquireStart();
    asynStatus acquireStop();
    asynStatus initializeDetector();
    LambdaSysImpl* lambdaInstance;
    int acquiredImages;
    int imageHeight;
    int imageWidth;
    int imageDepth;
    int queueDepth;
    bool acquiringData;
    int frameNumbersRead;
    long totalLossFramesRead;
    long latestImageNumberRead;
    size_t imageDims[2];
    char configFileName[256];
    NDArray *pImage;
    NDDataType_t imageDataType;
};

#define LAMBDA_VersionNumberString          "LAMBDA_VERSION_NUMBER"
#define LAMBDA_ConfigFilePathString         "LAMBDA_CONFIG_FILE_PATH"
#define LAMBDA_EnergyThresholdString        "LAMBDA_ENERGY_THRESHOLD"
#define LAMBDA_DecodedQueueDepthString      "LAMBDA_DECODED_QUEUE_DEPTH"
#define LAMBDA_OperatingModeString          "LAMBDA_OPERATING_MODE"
#define LAMBDA_DetectorStateString          "LAMBDA_DETECTOR_STATE"
#define LAMBDA_BadFrameCounterString       "LAMBDA_BAD_FRAME_COUNTER"
#define LAMBDA_BadImageString               "LAMBDA_BAD_IMAGE"


#define NUM_LAMBDA_PARAMS ((int)(&LAMBDA_LAST_PARAM - &LAMBDA_FIRST_PARAM + 1))

#endif
