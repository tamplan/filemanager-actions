#!/bin/sh

srcdir=$(cd ${0%/*}; pwd)

target=doc ${srcdir}/run-autogen.sh &&
	${srcdir}/tools/check-po.sh -nodummy &&
	${srcdir}/tools/check-headers.sh -nodummy &&
	make -C _build distcheck
