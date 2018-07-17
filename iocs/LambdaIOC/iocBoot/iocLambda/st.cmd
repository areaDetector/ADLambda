errlogInit(20000)

< envPaths
#epicsThreadSleep(20)
dbLoadDatabase("$(TOP)/dbd/LambdaApp.dbd")
LambdaApp_registerRecordDeviceDriver(pdbbase) 

# Prefix for all records
epicsEnvSet("PREFIX", "XF:10IDC-BI{Lambda-Cam:1}")
# The port name for the detector
epicsEnvSet("PORT",   "LAMBDA1")
# The queue size for all plugins
epicsEnvSet("QSIZE",  "30")
# The maximim image width; used for row profiles in the NDPluginStats plugin
epicsEnvSet("XSIZE",  "256")
# The maximim image height; used for column profiles in the NDPluginStats plugin
epicsEnvSet("YSIZE",  "256")
# The maximum number of time seried points in the NDPluginStats plugin
epicsEnvSet("NCHANS", "2048")
# The maximum number of frames buffered in the NDPluginCircularBuff plugin
epicsEnvSet("CBUFFS", "500")
# The search path for database files
epicsEnvSet("EPICS_DB_INCLUDE_PATH", "$(ADCORE)/db")
#epicsThreadSleep(15)
# Create a PICam driver
# LambdaConfig(const char *portName, const char * configPath, IDType, IDValue, maxBuffers, size_t maxMemory, int priority, int stackSize)

# This is for a
#/** Configuration command for Lambda driver; creates a new ADLambda object.
# * \param[in] portName The name of the asyn port driver to be created.
# * \param[in] configPath to the config files.
# * \param[in] maxBuffers The maximum number of NDArray buffers that the
# *            NDArrayPool for this driver is
# *            allowed to allocate. Set this to -1 to allow an unlimited number
# *            of buffers.
# * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for
# *            this driver is allowed to allocate. Set this to -1 to allow an
# *            unlimited amount of memory.
# * \param[in] priority The thread priority for the asyn port driver thread if
# *            ASYN_CANBLOCK is set in asynFlags.
# * \param[in] stackSize The stack size for the asyn port driver thread if
# *            ASYN_CANBLOCK is set in asynFlags.
# */
# config directory contains configuration files defined Globals.h, IP addresses, gains, and Medapix3 chip parameters
#int LambdaConfig(const char *portName, const char* configPath, int maxBuffers, size_t maxMemory, int priority, int stackSize) {
LambdaConfig("$(PORT)", "/localdata/config",  0, 0, 0, 0)
epicsThreadSleep(2)

asynSetTraceIOMask($(PORT), 0, 2)
#asynSetTraceMask($(PORT),0,0xff)

dbLoadRecords("$(ADCORE)/db/ADBase.template", "P=$(PREFIX),R=cam1:,PORT=$(PORT),ADDR=0,TIMEOUT=1")
dbLoadRecords("$(ADLAMBDA)/db/ADLambda.template","P=$(PREFIX),R=cam1:,PORT=$(PORT),ADDR=0,TIMEOUT=1")
#
# Create a standard arrays plugin, set it to get data from Driver.
#int NDStdArraysConfigure(const char *portName, int queueSize, int blockingCallbacks, const char *NDArrayPort, int NDArrayAddr, int maxBuffers, size_t maxMemory,
#                          int priority, int stackSize, int maxThreads)
NDStdArraysConfigure("Image1", 3, 0, "$(PORT)", 0)
#dbLoadRecords("$(ADCORE)/db/NDPluginBase.template","P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),NDARRAY_ADDR=0")
#dbLoadRecords("$(ADCORE)/db/NDStdArrays.template", "P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,TIMEOUT=1,TYPE=Int16,SIZE=16,FTVL=SHORT,NELEMENTS=802896")
dbLoadRecords("$(ADCORE)/db/NDStdArrays.template", "P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,NDARRAY_PORT=$(PORT),TIMEOUT=1,TYPE=Int16,FTVL=SHORT,NELEMENTS=1000000")
#
# Load all other plugins using commonPlugins.cmd
< $(ADCORE)/iocBoot/commonPlugins.cmd
#
#Note mpi control pipe out & in reversed.  Names are from the view of the MPI program.
#NDPipeWriterConfigure("PipeWriter1", 15000, 0, "$(PORT)", "/local/xpcscmdout", "/local/xpcscmdin", 0, 0, 0, 0,0)
#dbLoadRecords("$(ADCORE)/db/NDPluginPipeWriter.template", "P=$(PREFIX),R=PW1:,  PORT=PipeWriter1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),CMD_IN_PORT=PW_CMD_IN,CMD_OUT_PORT=PW_CMD_OUT")

#Note Local plugin to run the IMM plugin writer
#NDFileIMMConfigure("IMM1", 15000, 0, "$(PORT)",  0, 0, 0)
#dbLoadRecords("$(ADCORE)/db/NDFileIMM.template", "P=$(PREFIX),R=IMM1:,PORT=IMM1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT)")

set_requestfile_path("$(ADLAMBDA)/LambdaApp/Db")
#set_requestfile_path("$(NDPLUGINPIPEWRITER)/PipeWriterApp/Db")
#set_requestfile_path("$(NDPLUGINFILEIMM)/NDFileIMMApp/Db")

#asynSetTraceMask($(PORT),0,0x09)
#asynSetTraceMask($(PORT),0,0x11)
iocInit()

# save things every thirty seconds
create_monitor_set("auto_settings.req", 30, "P=$(PREFIX)")

