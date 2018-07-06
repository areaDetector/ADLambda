#!/bin/bash
export EPICS_CA_MAX_ARRAY_BYTES=10000000

#cd /local/epics/synApps_5_8/support/areaDetector-R2-5/ADLambda/iocs/LambdaIOC/iocBoot/iocLambda

../../bin/linux-x86_64/LambdaApp st.cmd
