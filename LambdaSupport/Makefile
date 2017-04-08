TOP=../
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================
SRC_DIRS += ../liblambda-$(EPICS_HOST_ARCH)/include/lambda
INC += DetInterface.h
INC += Globals.h
INC += LambdaGlobals.h
INC += LambdaInterface.h
INC += LambdaSysImpl.h

#=============================
ifeq (linux-x86_64, $(findstring linux-x86_64, $(T_A)))
LIB_INSTALLS_Linux += ../liblambda-$(EPICS_HOST_ARCH)/lib/liblambda.so
LIB_INSTALLS_Linux += ../liblambda-$(EPICS_HOST_ARCH)/lib/liblambda.so.0.5
LIB_INSTALLS_Linux += ../liblambda-$(EPICS_HOST_ARCH)/lib/liblambda.so.1
endif

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
