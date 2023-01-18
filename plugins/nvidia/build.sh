#! /bin/bash

g++ -O3 -fPIC --std=c++20 -shared $1/src/main.cc $1/src/reader.cc -lnvidia-ml -o $2/nvidia.so -I $1/../../common/src/
