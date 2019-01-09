#!/bin/tcsh
setenv EPICS_CA_MAX_ARRAY_BYTES 10000000
setenv LD_LIBRARY_PATH /local/boost-build/lib/:/local/epics/synApps_6_0/support/areaDetector-R3-4/ADLambda/lib/linux-x86_64
cd /local/epics/synApps_6_0/support/areaDetector-R3-4/ADLambda/iocs/LambdaIOC/iocBoot/iocLambda

../../bin/linux-x86_64/LambdaApp st.cmd
