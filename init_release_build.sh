#!/bin/sh

mkdir release
cd release
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
