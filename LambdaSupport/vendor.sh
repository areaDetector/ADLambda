#!/bin/bash
# Script that can be used to make the required vendor libraries for the ADLambda driver

echo "Create necessary directories"
mkdir liblambda-build
mkdir libfsdetcore-build
mkdir liblambda-linux-x86_64
mkdir libfsdetcore-linux-x86_64

echo  "Configure, make and install libfsdetcore"
cd libfsdetcore-build
cmake -DCMAKE_BUILD_TYPE=RELEASE -DCMAKE_INSTALL_PREFIX=../libfsdetcore-linux-x86_64 ../libfsdetcore
make
make install
export PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:/epics/support/areaDetector-3-3/ADLambda/LambdaSupport/libfsdetcore-linux-x86_64/lib/pkgconfig
cd ..

export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/epics/support/areaDetector-3-3/ADLambda/LambdaSupport/libfsdetcore-linux-x86_64/lib

echo "Configure, make and install liblambda"
cd liblambda-build
cmake -DCMAKE_BUILD_TYPE=RELEASE -DCMAKE_INSTALL_PREFIX=../liblambda-linux-x86_64 ../liblambda
make
make install
export PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:/epics/support/areaDetector-3-3/ADLambda/LambdaSupport/liblambda-linux-x86_64/lib/pkgconfig
cd ..

echo "Should be finished"
