#!/bin/sh
#
# Release a new Nautilus-Actions version
#
# (E): 1. (opt) "stable" will update the latest.tar.gz links

if [ ! -f Makefile ]; then
	echo "Makefile not found, probably not the good current directory" 1>&2
	exit 1
fi

thisdir=$(cd $(dirname $0); pwd)
product="$(grep -e '^PACKAGE_TARNAME' Makefile | awk '{ print $3 }')"
version="$(grep PACKAGE_VERSION Makefile | awk '{ print $3 }')"
tarname="${product}-${version}.tar.gz"

if [ ! -f "${tarname}" ]; then
	echo "${tarname} not found, do you have 'make distcheck' ?" 1>&2
	exit 1
fi

bstable=0
if [ "x$1" = "xstable" ]; then
	bstable=1
fi

echo -n "
Releasing "
[ ${bstable} -eq 1 ] && echo -n "stable" || echo -n "unstable"
echo " ${tarname}"

destdir="/net/data/tarballs/${product}"
echo " 
Installing in ${destdir}"
mkdir -p "${destdir}"
scp -v "${tarname}" "${destdir}/"
sha1sum ${tarname} > ${tarname}.sha1sum
if [ "${bstable}" -eq 1 ]; then
	echo "Updating ${destdir}/latest.tar.gz"
	(cd ${destdir}; rm -f latest.tar.gz; ln -s ${tarname} latest.tar.gz)
fi

echo " 
Installing on gnome.org"
scp "${tarname}" pwieser@master.gnome.org:
ssh pwieser@master.gnome.org install-module -u ${tarname}

echo " 
Installing on kimsufi"
destdir="/home/www/${product}/tarballs"
scp "${tarname}" root@kimsufi:${destdir}/
if [ "x$1" = "xstable" ]; then
	echo "Updating ${destdir}/latest.tar.gz"
	ssh root@kimsufi "cd ${destdir}; rm -f latest.tar.gz; ln -s ${tarname} latest.tar.gz"
fi
ssh root@kimsufi chown maintainer:users /home/www/nautilus-actions/tarballs/*

echo " 
Tagging git"
tag="$(echo ${product}-${version} | tr '[:lower:]' '[:upper:]' | sed -e 's/-/_/g' -e 's/\./_/g')"
msg="Releasing $(grep PACKAGE_NAME Makefile | awk '{ print $3 }') ${version}"
echo "git tag -s ${tag} -m ${msg}"
git tag -s "${tag}" -m "${msg}"
git pull --rebase && git push && git push --tags

echo "
Compressing local git repository"
git gc

echo "
Successfully ended. You may now send your mail.
"
