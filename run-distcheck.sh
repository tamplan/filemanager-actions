#!/bin/sh

srcdir=$(cd ${0%/*}; pwd)
builddir="_build"

# a nautilus-actions-x.y may remain after an aborted make distcheck
# such a directory breaks gnome-autogen.sh generation
# so clean it here
for d in $(find ${srcdir} -maxdepth 2 -type d -name 'nautilus-actions-*'); do
	chmod -R u+w $d
	rm -fr $d
done

rm -fr ${builddir}
rm -fr _install
find ${srcdir}/docs/nact -type f -name '*.html' -o -name '*.pdf' | xargs rm -f
find ${srcdir}/docs/nact \( -type d -o -type l \) -name 'stylesheet-images' -o -name 'admon' | xargs rm -fr

target=doc ${srcdir}/run-autogen.sh &&
	${srcdir}/tools/check-po.sh -nodummy &&
	${srcdir}/tools/check-headers.sh -nodummy -builddir=${builddir} &&
	make -C ${builddir} distcheck
