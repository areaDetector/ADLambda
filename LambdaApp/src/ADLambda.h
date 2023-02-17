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
#include <map>
#include <deque>
#include <variant>


#include <epicsString.h>
#include <epicsEvent.h>
#include <epicsThread.h>

#include "ADDriver.h"

static const int ONE_BIT = 1;
static const int SIX_BIT = 6;
static const int TWELVE_BIT = 12;
static const int TWENTY_FOUR_BIT = 24;

static const double ONE_BILLION = 1.E9;

static const double SHORT_TIME = 0.000025;

typedef std::variant<std::shared_ptr<xsp::lambda::Receiver>, std::shared_ptr<xsp::PostDecoder> > lambda_input;

/**
 * Class to wrap Lambda detector library provided by X-Spectrum
 */
class epicsShareClass ADLambda: public ADDriver 
{
public:
	static const char *driverName;

	ADLambda(const char *portName, const char *configPath, int numModules, int fake);
	~ADLambda();

	virtual asynStatus disconnect();
	virtual asynStatus connect();
	
	void waitAcquireThread();
	void tryConnect();
	void acquireThread(int receiver);
	void acquireDecoderThread();
	void exportThread();

	void report(FILE *fp, int details);

	virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);

protected:
    int LAMBDA_ConfigFilePath;
    #define LAMBDA_FIRST_PARAM LAMBDA_ConfigFilePath
    int LAMBDA_DecoderDetected;
    int LAMBDA_EnergyThreshold;
    int LAMBDA_DualThreshold;
    int LAMBDA_DecodedQueueDepth;
    int LAMBDA_OperatingMode;
    int LAMBDA_DualMode;
    int LAMBDA_ChargeSumming;
    int LAMBDA_GatingEnable;
    int LAMBDA_BadFrameCounter;
    int LAMBDA_BadImage;
    int LAMBDA_ReadoutThreads;
    int LAMBDA_StitchedWidth;
    int LAMBDA_StitchedHeight;

private:
	bool connected = false;
	bool hasDecoder = false;

   	void setSizes();
   	void incrementValue(int param);
   	void decrementValue(int param);
   	void readParameters();
   	void sendParameters();
   	void writeDepth(int depth);

	bool tryStartAcquire();
	bool tryStopAcquire();
	
	int fake;

	void spawnAcquireThread(int receiver);
	void spawnAcquireDecoderThread();

	std::unique_ptr<xsp::System> sys;
	std::shared_ptr<xsp::lambda::Detector> det;
	
	std::vector< lambda_input > inputs;
	
	std::map<int, NDArray*> frames;
	std::deque<NDArray*> export_queue;
	
	epicsEvent* startAcquireEvent;
	epicsEvent* stopAcquireEvent;
 	epicsEvent** threadFinishEvents;
 	epicsMutex* dequeLock;

    std::string configFileName;
    NDArray *pImage = NULL;
    NDArray** saved_frames;
    NDDataType_t imageDataType;
};

typedef struct
{
	ADLambda* driver;
	int receiver;
} acquire_data;


#define LAMBDA_ConfigFilePathString         "LAMBDA_CONFIG_FILE_PATH"
#define LAMBDA_DecoderDetectedString        "LAMBDA_DECODER_DETECTED"
#define LAMBDA_EnergyThresholdString        "LAMBDA_ENERGY_THRESHOLD"
#define LAMBDA_DualThresholdString          "LAMBDA_DUAL_THRESHOLD"
#define LAMBDA_DecodedQueueDepthString      "LAMBDA_DECODED_QUEUE_DEPTH"
#define LAMBDA_OperatingModeString          "LAMBDA_OPERATING_MODE"
#define LAMBDA_DualModeString               "LAMBDA_DUAL_MODE"
#define LAMBDA_ChargeSummingString          "LAMBDA_CHARGE_SUMMING"
#define LAMBDA_GatingEnableString           "LAMBDA_GATING_ENABLE"
#define LAMBDA_BadFrameCounterString        "LAMBDA_BAD_FRAME_COUNTER"
#define LAMBDA_BadImageString               "LAMBDA_BAD_IMAGE"
#define LAMBDA_ReadoutThreadsString         "LAMBDA_NUM_READOUT_THREADS"
#define LAMBDA_StitchWidthString            "LAMBDA_STITCHED_WIDTH"
#define LAMBDA_StitchHeightString           "LAMBDA_STITCHED_HEIGHT"


#endif
