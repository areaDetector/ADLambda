/**
 Copyright (c) 2015, UChicago Argonne, LLC
 See LICENSE file.
*/
/* ADLambda.cpp */
#include <iocsh.h>

#include <epicsExit.h>
#include <epicsExport.h>

#include "ADLambda.h"


extern "C" {
/** Configuration command for PICAM driver; creates a new PICam object.
 * \param[in] portName The name of the asyn port driver to be created.
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
	int LambdaConfig(const char *portName, int maxBuffers,
			size_t maxMemory, int priority, int stackSize) {
		new ADLambda(portName, maxBuffers, maxMemory, priority, stackSize);
		return (asynSuccess);
	}

	/**
	 * Callback function for exit hook
	 */
	static void exitCallbackC(void *pPvt){
		ADLambda *pADLambda = (ADLambda*)pPvt;
		delete pADLambda;
	}

}

const char *ADLambda::driverName = "Lambda";

/**
 * Constructor
 *  * \param[in] portName The name of the asyn port driver to be created.
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
ADLambda::ADLambda(const char *portName, int maxBuffers, size_t maxMemory,
		int priority, int stackSize) :
		ADDriver(portName, 1, int(NUM_LAMBDA_PARAMS), maxBuffers, maxMemory,
		asynEnumMask, asynEnumMask, ASYN_CANBLOCK, 1, priority, stackSize)
{

}

/**
 * Destructor
 */
ADLambda::~ADLambda()
{

}

void ADLambda::report(FILE *fp, int details)
{

}

asynStatus ADLambda::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
	int status = asynSuccess;

	return (asynStatus)status;
}

/* Code for iocsh registration */

/* PICamConfig */
static const iocshArg LambdaConfigArg0 = { "Port name", iocshArgString };
static const iocshArg LambdaConfigArg1 = { "maxBuffers", iocshArgInt };
static const iocshArg LambdaConfigArg2 = { "maxMemory", iocshArgInt };
static const iocshArg LambdaConfigArg3 = { "priority", iocshArgInt };
static const iocshArg LambdaConfigArg4 = { "stackSize", iocshArgInt };
static const iocshArg * const LambdaConfigArgs[] = { &LambdaConfigArg0,
        &LambdaConfigArg1, &LambdaConfigArg2, &LambdaConfigArg3, &LambdaConfigArg4 };

static void configLambdaCallFunc(const iocshArgBuf *args){
	LambdaConfig(args[0].sval, args[1].ival, args[2].ival, args[3].ival,
			args[4].ival);
}
static const iocshFuncDef configLambda = { "LambdaConfig", 5, LambdaConfigArgs };

static void LambdaRegister(void) {
	iocshRegister(&configLambda, configLambdaCallFunc);
}

extern "C" {
epicsExportRegistrar(LambdaRegister);
}
