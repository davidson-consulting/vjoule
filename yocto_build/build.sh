#!/usr/bin/env bash

if [[ ! -f ./libyocto-static.a ]]
then
    wget https://www.yoctopuce.com/FR/downloads/YoctoLib.cpp.57762.zip -O yocto.zip
    mkdir src
    mv yocto.zip src
    cd src ; unzip yocto.zip ; cd Binaries
    sed -i 's/OPTS_STAT = /OPTS_STAT = -fPIC/g' GNUmakefile
    make linux/64bits/libyocto-static.a
    cd ../..
    cp src/Binaries/linux/64bits/libyocto-static.a libyocto-static.a
fi

echo "[100%] yocto-static.a build"
