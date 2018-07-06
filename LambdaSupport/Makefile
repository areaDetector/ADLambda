TOP=../
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================
#SRC_DIRS += ../liblambda-$(EPICS_HOST_ARCH)/include/lambda
SRC_DIRS += ../liblambda-$(EPICS_HOST_ARCH)/include
SRC_DIRS += ../libfsdetcore-$(EPICS_HOST_ARCH)/include
#INC += DetInterface.h
INC += fsdetector/lambda/Version.h
INC += fsdetector/lambda/LambdaGlobals.h
INC += fsdetector/lambda/LambdaInterface.h
INC += fsdetector/lambda/LambdaSysImpl.h
INC += fsdetector/core/Globals.h
INC += fsdetector/core/NetworkInterface.h
INC += fsdetector/core/NetworkImplementation.h
INC += fsdetector/core/Utils.h
INC += fsdetector/core/FilesOperation.h
INC += fsdetector/core/ThreadUtils.h
INC += fsdetector/core/MemUtils.h
INC += fsdetector/core/Version.h
INC += fsdetector/core/MemUtils.tpp

#=============================
ifeq (linux-x86_64, $(findstring linux-x86_64, $(T_A)))
# the .so files should be links to .1.0 and .0.9 respectively
LIB_INSTALLS_Linux += ../liblambda-$(EPICS_HOST_ARCH)/lib/liblambda.so
LIB_INSTALLS_Linux += ../libfsdetcore-$(EPICS_HOST_ARCH)/lib/libfsdetcore.so
LIB_INSTALLS_Linux += ../liblambda-$(EPICS_HOST_ARCH)/lib/liblambda.so.1.0
LIB_INSTALLS_Linux += ../libfsdetcore-$(EPICS_HOST_ARCH)/lib/libfsdetcore.so.0.9
endif

vpath %.tpp $(USR_VPATH) $(ALL_SRC_DIRS) include 
include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
