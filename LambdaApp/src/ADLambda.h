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

static const int ONE_BIT = 1;
static const int SIX_BIT = 6;
static const int TWELVE_BIT = 12;
static const int TWENTY_FOUR_BIT = 24;

static const int ONE_BIT_MODE = 0;
static const int SIX_BIT_MODE = 1;
static const int TWELVE_BIT_MODE = 2;
static const int TWENTY_FOUR_BIT_MODE = 3;

static const double ONE_BILLION = 1.E9;

/**
 * Class to wrap Lambda detector library provided by X-Spectrum
 */
class epicsShareClass ADLambda: public ADDriver 
{
public:
	static const char *driverName;

	ADLambda(const char *portName, const char *configPath, int numModules, int readout);
	~ADLambda();

	virtual asynStatus disconnect();
	virtual asynStatus connect();
	
	void waitAcquireThread();
	void acquireThread(int receiver, int thread_no);
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
    int LAMBDA_EnergyThresholdRBV;
    int LAMBDA_DualThreshold;
    int LAMBDA_DualThresholdRBV;
    int LAMBDA_DecodedQueueDepth;
    int LAMBDA_OperatingMode;
    int LAMBDA_DetectorState;
    int LAMBDA_BadFrameCounter;
    int LAMBDA_MedipixIDs;
    int LAMBDA_DetCoreVersionNumber;
    int LAMBDA_BadImage;
    int LAMBDA_ReadoutThreads;
#define LAMBDA_LAST_PARAM LAMBDA_ReadoutThreads

private:
	bool connected;
	int ReadThreadPerModule;

    asynStatus acquireStart();
    asynStatus acquireStop();
    asynStatus initializeDetector();
    asynStatus setSizeParams();
   
   	void getThresholds();
   
	void spawnAcquireThread(int receiver, int thread_no);

	std::unique_ptr<xsp::System> sys;
	std::shared_ptr<xsp::lambda::Detector> det;
	
	std::vector<std::shared_ptr<xsp::Receiver> > recs;
	
	epicsEvent* startAcquireEvent;
 	epicsEvent** threadFinishEvents;
 	epicsMutex** threadReceiverLocks;

    std::string configFileName;
    NDArray *pImage;
    NDArray** saved_frames;
    NDDataType_t imageDataType;
};

typedef struct
{
	ADLambda* driver;
	int receiver;
	int thread_no;
} acquire_data;


#define LAMBDA_VersionNumberString          "LAMBDA_VERSION_NUMBER"
#define LAMBDA_ConfigFilePathString         "LAMBDA_CONFIG_FILE_PATH"
#define LAMBDA_EnergyThresholdString        "LAMBDA_ENERGY_THRESHOLD"
#define LAMBDA_EnergyThresholdRBVString     "LAMBDA_ENERGY_THRESHOLD_READ"
#define LAMBDA_DualThresholdString          "LAMBDA_DUAL_THRESHOLD"
#define LAMBDA_DualThresholdRBVString       "LAMBDA_DUAL_THRESHOLD_READ"
#define LAMBDA_DecodedQueueDepthString      "LAMBDA_DECODED_QUEUE_DEPTH"
#define LAMBDA_OperatingModeString          "LAMBDA_OPERATING_MODE"
#define LAMBDA_DetectorStateString          "LAMBDA_DETECTOR_STATE"
#define LAMBDA_BadFrameCounterString        "LAMBDA_BAD_FRAME_COUNTER"
#define LAMBDA_MedipixIDsString             "LAMBDA_MEDIPIX_IDS"
#define LAMBDA_DetCoreVersionNumberString   "LAMBDA_DET_CORE_VERSION"
#define LAMBDA_BadImageString               "LAMBDA_BAD_IMAGE"
#define LAMBDA_ReadoutThreadsString         "LAMBDA_NUM_READOUT_THREADS"
#define LAMBDA_TemperatureString            "LAMBDA_TEMPERATURE"


#define NUM_LAMBDA_PARAMS ((int)(&LAMBDA_LAST_PARAM - &LAMBDA_FIRST_PARAM + 1))

#endif
