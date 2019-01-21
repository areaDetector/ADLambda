TOP=../
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================
#INSTALL_LOCATION = $(TOP)
#INSTALL_LOCATION_LIB = $(TOP)/lib
SRC_DIRS += ../liblambda-$(EPICS_HOST_ARCH)/include
INC += fsdetector/core/FilesOperation.h
INC += fsdetector/core/Globals.h
INC += fsdetector/core/MemUtils.h
INC += fsdetector/core/MemUtils.tpp
INC += fsdetector/core/NetworkImplementation.h
INC += fsdetector/core/NetworkInterface.h
INC += fsdetector/core/ThreadUtils.h
INC += fsdetector/core/Utils.h
INC += fsdetector/core/Version.h
INC += fsdetector/lambda/LambdaGlobals.h
INC += fsdetector/lambda/LambdaInterface.h
INC += fsdetector/lambda/LambdaSysImpl.h
INC += fsdetector/lambda/Version.h
#=============================
ifeq (linux-x86_64, $(findstring linux-x86_64, $(T_A)))
LIB_INSTALLS_Linux += ../liblambda-$(EPICS_HOST_ARCH)/lib/liblambda.so.0.9
LIB_INSTALLS_Linux += ../liblambda-$(EPICS_HOST_ARCH)/lib/liblambda.so
LIB_INSTALLS_Linux += ../liblambda-$(EPICS_HOST_ARCH)/lib/libfsdetcore.so.0.8
LIB_INSTALLS_Linux += ../liblambda-$(EPICS_HOST_ARCH)/lib/libfsdetcore.so
endif

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
