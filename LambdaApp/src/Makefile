TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

ifeq (windows-x64, $(findstring windows-x64, $(T_A)))
USR_CPPFLAGS += -D__X64
endif

USR_CPPFLAGS += -std=c++11

LAMBDA_HOME = /epics/support/areaDetector-3-3/ADLambda
USR_INCLUDES += -I$(LAMBDA_HOME)/include/fsdetector/lambda
# USR_INCLUDES += -I$(LAMBDA_HOME)/include/fsdetector/core

LIBRARY_IOC = ADLambda
LIB_SRCS += ADLambda.cpp
LIB_LIBS += lambda


DBD += LambdaSupport.dbd

include $(ADCORE)/ADApp/commonLibraryMakefile

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

