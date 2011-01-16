#!/bin/sh

./run-autogen.sh &&
./tools/check-po.sh -nodummy &&
./tools/check-headers.sh -nodummy &&
make &&
make distcheck
