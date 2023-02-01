#! /bin/bash

if [ ! -f $2/nvidia.so ]
then
   g++ -O3 -fPIC --std=c++20 -shared $1/src/main.cc $1/src/reader.cc -lnvidia-ml -o $2/nvidia.so -I $1/../../common/src/
elif [ $1/src/main.cc -nt $2/nvidia.so ]
then
    g++ -O3 -fPIC --std=c++20 -shared $1/src/main.cc $1/src/reader.cc -lnvidia-ml -o $2/nvidia.so -I $1/../../common/src/
fi
