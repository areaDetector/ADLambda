#!/usr/bin/env perl

mkdir libfsdetcore-build
mkdir libfsdetcore-linux-x86_64

cd libfsdetcore-build

cmake -DCMAKE_INSTALL_PREFIX=../libfsdetcore-linux-x86_64 ../libfsdetcore
make
make install

cd ..

mkdir liblambda-build
mkdir liblambda-linux-x86_64

cd liblambda-build

cmake -DCMAKE_INSTALL_PREFIX=../liblambda-linux-x86_64 ../liblambda
make
make install

cd ..

