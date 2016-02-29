#!/bin/tcsh
cd /local/epics/synApps_5_8/support/areaDetector-R2-0/ADLambda/iocs/LambdaIOC/iocBoot/iocLambda
setenv EPICS_DISPLAY_PATH ../../../../LambdaApp/op/adl:../../../../../ADCore-R2-2/ADApp/op/adl:../../../../../NDPluginPipeWriter/PipeWriterApp/op/adl:../../../../../../asyn-4-26/opi/medm

medm -x -macro "P=8LAMBDA1:,R=cam1:" LambdaBase.adl &
sleep 1
medm -x -macro "P=8LAMBDA1:,R=PW1:" NDPluginPipeWriter.adl &
#medm -attach -x -macro "P=8LAMBDA1:,R=PW1:" NDPluginPipeWriter.adl &
#medm -attach -x -macro "P=8LAMBDA1:,R=PW1:" MPI.adl &

