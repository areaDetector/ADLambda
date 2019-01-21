errlogInit(20000)

< envPaths
#epicsThreadSleep(20)
dbLoadDatabase("$(TOP)/dbd/LambdaApp.dbd")
LambdaApp_registerRecordDeviceDriver(pdbbase) 

# Prefix for all records
epicsEnvSet("PREFIX", "8LAMBDA1:")
# The port name for the detector
epicsEnvSet("PORT",   "LAMBDA1")
# The queue size for all plugins
epicsEnvSet("QSIZE",  "30")
# The maximim image width; used for row profiles in the NDPluginStats plugin
epicsEnvSet("XSIZE",  "2048")
# The maximim image height; used for column profiles in the NDPluginStats plugin
epicsEnvSet("YSIZE",  "2048")
# The maximum number of time seried points in the NDPluginStats plugin
epicsEnvSet("NCHANS", "2048")
# The maximum number of frames buffered in the NDPluginCircularBuff plugin
epicsEnvSet("CBUFFS", "1000")
# The search path for database files
epicsEnvSet("EPICS_DB_INCLUDE_PATH", "$(ADCORE)/db")
#epicsThreadSleep(15)
# Create a PICam driver
# LambdaConfig(const char *portName, const char * configPath, IDType, IDValue, maxBuffers, size_t maxMemory, int priority, int stackSize)

# This is for a 
LambdaConfig("$(PORT)", "config",  0, 0, 0, 0)
epicsThreadSleep(2)

asynSetTraceIOMask($(PORT), 0, 2)
#asynSetTraceMask($(PORT),0,0xff)

dbLoadRecords("$(ADCORE)/db/ADBase.template", "P=$(PREFIX),R=cam1:,PORT=$(PORT),ADDR=0,TIMEOUT=1")
dbLoadRecords("$(ADLAMBDA)/db/ADLambda.template","P=$(PREFIX),R=cam1:,PORT=$(PORT),ADDR=0,TIMEOUT=1")
#
# Create a standard arrays plugin, set it to get data from Driver.
NDStdArraysConfigure("Image1", 3, 0, "$(PORT)", 0)
dbLoadRecords("$(ADCORE)/db/NDPluginBase.template","P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),NDARRAY_ADDR=0")
dbLoadRecords("$(ADCORE)/db/NDStdArrays.template", "P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),TYPE=Int16,FTVL=SHORT,NELEMENTS=802896")
#
# Load all other plugins using commonPlugins.cmd
< $(ADCORE)/iocBoot/commonPlugins.cmd
#
dbpf "8LAMBDA1:Scatter1:MaxThreads", "5"

#Note Local plugin to run the IMM plugin writer
NDFileIMMConfigure("IMM0", 2000000, 300, 0, "$(PORT)",  0, 0, 0)
dbLoadRecords("$(ADCORE)/db/NDFileIMM.template", "P=$(PREFIX),R=IMM0:,PORT=IMM0,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT)")
NDFileIMMConfigure("IMM1", 2000000, 300, 0, "$(PORT)",  0, 0, 0 )
dbLoadRecords("$(ADCORE)/db/NDFileIMM.template", "P=$(PREFIX),R=IMM1:,PORT=IMM1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT)")
NDFileIMMConfigure("IMM2", 2000000, 300, 0, "$(PORT)",  0, 0, 0)
dbLoadRecords("$(ADCORE)/db/NDFileIMM.template", "P=$(PREFIX),R=IMM2:,PORT=IMM2,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT)")
NDFileIMMConfigure("IMM3", 2000000, 300, 0, "$(PORT)",  0, 0, 0)
dbLoadRecords("$(ADCORE)/db/NDFileIMM.template", "P=$(PREFIX),R=IMM3:,PORT=IMM3,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT)")
NDFileIMMConfigure("IMMout", 2000000, 300, 0, "$(PORT)",  0, 0, 0)
dbLoadRecords("$(ADCORE)/db/NDFileIMM.template", "P=$(PREFIX),R=IMMout:,PORT=IMMout,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT)")

dbLoadRecords("$(TOP)/iocBoot/iocLambda/IMMJoin.template", "P=$(PREFIX), R=IMMJoin:")
set_requestfile_path("$(ADLAMBDA)/LambdaApp/Db")
set_requestfile_path("$(NDPLUGINPIPEWRITER)/PipeWriterApp/Db")
set_requestfile_path("$(NDPLUGINFILEIMM)/NDFileIMMApp/Db")

#asynSetTraceMask($(PORT),0,0x09)
#asynSetTraceMask($(PORT),0,0x11)
iocInit()

# save things every thirty seconds
create_monitor_set("auto_settings.req", 30, "P=$(PREFIX)")


epicsThreadSleep "1"
dbpf "8LAMBDA1:cam1:TriggerMode", "0"
dbpf "8LAMBDA1:cam1:AcquireTime", ".001"
dbpf "8LAMBDA1:cam1:AcquirePeriod", ".001"
dbpf "8LAMBDA1:cam1:NumImages", "4735"
epicsThreadSleep "1"

dbpf "8LAMBDA1:cam1:DataType", "3"
epicsThreadSleep "1"

# Initialize the NDFileIMM_format for the IMM  plugins since not set on iocInit
dbpf "8LAMBDA1:IMM0:NDFileIMM_format.PROC", "1"
dbpf "8LAMBDA1:IMM1:NDFileIMM_format.PROC", "1"
dbpf "8LAMBDA1:IMM2:NDFileIMM_format.PROC", "1"
dbpf "8LAMBDA1:IMM3:NDFileIMM_format.PROC", "1"
dbpf "8LAMBDA1:IMMout:NDFileIMM_format.PROC", "1"
# Initialize the EnergyThreshold on the Lambda since this does not happen on iocInit
dbpf "8LAMBDA1:cam1:EnergyThreshold.PROC", "1"
epicsThreadSleep "1"
# Collect some images to initialize IMM0-IMM3.
dbpf "8LAMBDA1:cam1:Acquire", "1"
epicsThreadSleep "2"
dbpf "8LAMBDA1:cam1:Acquire", "0"
# Use the IMM Join which uses dfanout to set IMM0-IMM3 to Capture
dbpf "8LAMBDA1:IMMJoin:IMMJoinedCapture", "1"
epicsThreadSleep "2"
#Collect a few images to Initialize the IMMout plugin
dbpf "8LAMBDA1:cam1:Acquire", "1"
epicsThreadSleep "2"
dbpf "8LAMBDA1:cam1:Acquire", "0"
# All ready for users to take some images.

