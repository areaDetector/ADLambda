TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

GIT_EXISTS := $(shell git rev-parse --is-inside-work-tree 2>/dev/null)
ifeq ($(GIT_EXISTS), true)
USR_CPPFLAGS += -DGIT_VERSION="\"$(shell git describe --always --tags)\""
endif

ifeq (windows-x64, $(findstring windows-x64, $(T_A)))
USR_CPPFLAGS += -D__X64
endif

USR_CPPFLAGS += -fpermissive
LIBRARY_IOC = ADLambda
LIB_SRCS += ADLambda.cpp
USR_SYS_LIBS += xsp

DBD += LambdaSupport.dbd

include $(ADCORE)/ADApp/commonLibraryMakefile

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

