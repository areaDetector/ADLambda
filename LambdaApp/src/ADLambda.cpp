/**
 Copyright (c) 2015, UChicago Argonne, LLC
 See LICENSE file.
*/
/* ADLambda.cpp */

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
		ADLambda::
