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
#include <libxsp.h>
#include <string>


#include <epicsString.h>
#include <epicsEvent.h>
#include <epicsThread.h>

#include "ADDriver.h"

/**
 * Class to wrap Lambda detector library provided by X-Spectrum
 */
class epicsShareClass ADLambda: public ADDriver 
{
public:
	static const char *driverName;
	static const int TWELVE_BIT, TWENTY_FOUR_BIT;

	ADLambda(const char *portName, const char *configPath, int numModules);
	~ADLambda();

	virtual asynStatus disconnect();
	virtual asynStatus connect();
	
	void processTwelveBit(const void*, NDArrayInfo);
	void processTwentyFourBit(const void*, NDArrayInfo);
	
	void waitAcquireThread();
	void acquireThread(int receiver);
	void monitorThread();

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
    int LAMBDA_MedipixIDs;
    int LAMBDA_DetCoreVersionNumber;
    int LAMBDA_BadImage;
#define LAMBDA_LAST_PARAM LAMBDA_BadImage

private:
	bool connected;

    asynStatus acquireStart();
    asynStatus acquireStop();
    asynStatus initializeDetector();
    asynStatus setSizeParams();

	std::unique_ptr<xsp::System> sys;
	std::shared_ptr<xsp::lambda::Detector> det;
	
	std::vector<std::shared_ptr<xsp::Receiver> > recs;
	
	epicsEvent* startAcquireEvent;
 	epicsEvent* threadFinishEvent;

    std::string configFileName;
    NDArray *pImage;
    NDDataType_t imageDataType;
};

#define LAMBDA_VersionNumberString          "LAMBDA_VERSION_NUMBER"
#define LAMBDA_ConfigFilePathString         "LAMBDA_CONFIG_FILE_PATH"
#define LAMBDA_EnergyThresholdString        "LAMBDA_ENERGY_THRESHOLD"
#define LAMBDA_DecodedQueueDepthString      "LAMBDA_DECODED_QUEUE_DEPTH"
#define LAMBDA_OperatingModeString          "LAMBDA_OPERATING_MODE"
#define LAMBDA_DetectorStateString          "LAMBDA_DETECTOR_STATE"
#define LAMBDA_BadFrameCounterString        "LAMBDA_BAD_FRAME_COUNTER"
#define LAMBDA_MedipixIDsString             "LAMBDA_MEDIPIX_IDS"
#define LAMBDA_DetCoreVersionNumberString   "LAMBDA_DET_CORE_VERSION"
#define LAMBDA_BadImageString               "LAMBDA_BAD_IMAGE"
#define LAMBDA_TemperatureString            "LAMBDA_TEMPERATURE"


#define NUM_LAMBDA_PARAMS ((int)(&LAMBDA_LAST_PARAM - &LAMBDA_FIRST_PARAM + 1))

#endif
