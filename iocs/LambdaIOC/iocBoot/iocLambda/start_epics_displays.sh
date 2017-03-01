#!/bin/tcsh
cd /local/epics/synApps_5_8/support/areaDetector-R2-5/ADLambda/iocs/LambdaIOC/iocBoot/iocLambda
setenv EPICS_DISPLAY_PATH ../../../../LambdaApp/op/adl:../../../../../ADCore-R2-5/ADApp/op/adl:../../../../../NDPluginPipeWriter/PipeWriterApp/op/adl:../../../../../../asyn-4-30/opi/medm:../../../../../NDPluginFileIMM/NDFileIMMApp/op/adl

medm -x -macro "P=8LAMBDA1:,R=cam1:" LambdaBase.adl &
sleep 1
medm -x -macro "P=8LAMBDA1:,R=PW1:" NDPluginPipeWriter.adl &
#medm -attach -x -macro "P=8LAMBDA1:,R=PW1:" NDPluginPipeWriter.adl &
#medm -attach -x -macro "P=8LAMBDA1:,R=PW1:" MPI.adl &

