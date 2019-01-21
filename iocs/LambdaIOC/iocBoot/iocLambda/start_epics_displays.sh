#!/bin/tcsh
cd /local/epics/synApps_6_0/support/areaDetector-R3-4/ADLambda/iocs/LambdaIOC/iocBoot/iocLambda
setenv EPICS_DISPLAY_PATH ../../../../LambdaApp/op/adl:../../../../../ADCore-R3-4/ADApp/op/adl:../../../../../../asyn-R4-33/opi/medm:../../../../../NDPluginFileIMM/NDFileIMMApp/op/adl

medm -x -macro "P=8LAMBDA1:,R=cam1:" LambdaBase.adl &
sleep 1
#medm -x -attach -macro "P=8LAMBDA1:,R=PW1:" NDPluginPipeWriter.adl &
medm -x -macro "P=8LAMBDA1:,R=cam1:" LambdaCollect.adl &
#medm -attach -x  "P=8LAMBDA1:,R=PW1:" NDPluginPipeWriter.adl &
#medm -attach -x  "P=8LAMBDA1:,R=PW1:" MPI.adl &

