/**
 Copyright (c) 2015, UChicago Argonne, LLC
 See LICENSE file.
*/
/* PICam.h
 *
 * This is an areaDetector driver for cameras that communicate
 * with the Priceton Instruments PICAM driver library
 *
 */
#ifndef ADPICAM_H
#define ADPICAM_H

#include "ADDriver.h"

class epicsShareClass ADLambda: public ADDriver {
public:
	static const char *driverName;

	ADLambda(const char *portName, int maxBuffers, size_t, maxMemory,
			int priority, int stackSize);
	~ADLambda();
    void report(FILE *fp, int details);
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);

protected:
    // int reference to parameters
    int LAMBDA_VersionNumber;
#define LAMBDA_FIRST_PARAM PICAM_VersionNumber
#define LAMBDA_LAST_PARAM PICAM_SensorTemperatureStatusRelevant

private:
    asynStatus initializeDetector();
};

#define LAMBDA_VersionNumberString          "LAMBDA_VERSION_NUMBER"

#define NUM_LAMBDA_PARAMS ((int)(&LAMBDA_LAST_PARAM - &LAMBDA_FIRST_PARAM + 1))

#endif
