TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

PROD_NAME = LambdaApp
PROD_IOC += $(PROD_NAME)

# <name>.dbd will be created from <name>Include.dbd
DBD += $(PROD_NAME).dbd
$(PROD_NAME)_DBD += LambdaSupport.dbd

# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
$(PROD_NAME)_SRCS += $(PROD_NAME)_registerRecordDeviceDriver.cpp $(PROD_NAME)Main.cpp

# Add locally compiled object code
$(PROD_NAME)_LIBS += ADLambda
# We use SYS_LIBS here because liblambda is only built dynamically but we want to be able to build
# either statically or dynamically
$(PROD_NAME)_SYS_LIBS += lambda
$(PROD_NAME)_SYS_LIBS += fsdetcore
$(PROD_NAME)_SYS_LIBS += boost_system
$(PROD_NAME)_SYS_LIBS += boost_thread
$(PROD_NAME)_SYS_LIBS += boost_chrono
$(PROD_NAME)_SYS_LIBS += boost_date_time
$(PROD_NAME)_SYS_LIBS += boost_system-mt
$(PROD_NAME)_SYS_LIBS += boost_thread-mt
$(PROD_NAME)_SYS_LIBS += boost_chrono-mt
$(PROD_NAME)_SYS_LIBS += boost_date_time-mt
$(PROD_NAME)_SYS_LIBS += boost_atomic-mt
$(PROD_NAME)_SYS_LIBS += boost_date_time-mt

USR_LDFLAGS += -L/local/boost-build/lib

LIB_LIBS_Linux += boost_system-mt
include $(ADCORE)/ADApp/commonDriverMakefile

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

