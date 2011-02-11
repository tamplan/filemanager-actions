#!/bin/sh

./run-autogen.sh &&
make clean &&
make &&
make install &&
./tools/check-po.sh -nodummy &&
./tools/check-headers.sh -nodummy &&
make distcheck
