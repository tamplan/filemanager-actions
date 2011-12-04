#!/bin/sh

srcdir=$(cd ${0%/*}; pwd)
builddir="_build"

rm -fr ${builddir}
rm -fr _install
find ${srcdir}/docs/nact -type f -name '*.html' -o -name '*.pdf' | xargs rm -f
find ${srcdir}/docs/nact \( -type d -o -type l \) -name 'stylesheet-images' -o -name 'admon' | xargs rm -fr

target=doc ${srcdir}/run-autogen.sh &&
	${srcdir}/tools/check-po.sh -nodummy &&
	${srcdir}/tools/check-headers.sh -nodummy -builddir=${builddir} &&
	make -C ${builddir} distcheck
