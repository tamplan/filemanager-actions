#!/bin/sh

srcdir=$(cd ${0%/*}; pwd)

target=doc ${srcdir}/run-autogen.sh &&
	make clean &&
	make &&
	make install &&
	${srcdir}/tools/check-po.sh -nodummy &&
	${srcdir}/tools/check-headers.sh -nodummy &&
	make distcheck
