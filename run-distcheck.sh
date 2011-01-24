#!/bin/sh

./run-autogen.sh &&
make &&
./tools/check-po.sh -nodummy &&
./tools/check-headers.sh -nodummy &&
make distcheck
