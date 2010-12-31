#!/bin/sh
#
# Release a new Nautilus-Actions version
#

if [ ! -f configure.ac ]; then
	echo "configure.ac not found, probably not the good current directory" 1>&2
	exit 1
fi
if [ "$(basename $(pwd))" != "nautilus-actions" ]; then
	echo "current directory is $(pwd): change to nautilus-actions" 1>&2
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

minor=$(echo ${version} | cut -d. -f2)
let minor_x=${minor}/2*2
[ ${minor} -eq ${minor_x} ] && bstable=1 || bstable=0

echo -n "
Releasing "
[ ${bstable} -eq 1 ] && echo -n "stable" || echo -n "unstable"
echo " ${tarname}"

echo -n "Are you OK to release (y/N) ? "
while [ 1 ]; do
	read -n1 -s key
	key=$(echo $key | tr '[:upper:]' '[:lower:]')
	[ "$key" = "y" -o "$key" = "n" -o "$key" = "" ] && break
done
[ "$key" = "y" ] && echo "Yes" || echo "No"
[ "$key" != "y" ] && exit

# are we local ?
destdir="/net/data/tarballs/${product}"
desthost="stormy.trychlos.org"
local=1
[ "$(ls ${destdir} 2>/dev/null)" = "" ] && local=0
echo " 
Installing in ${destdir}"
cmd="mkdir -p "${destdir}""
[ ${local} -eq 0 ] && ssh ${desthost} "${cmd}" || ${cmd}
[ ${local} -eq 0 ] && scp -v "${tarname}" "${desthost}:${destdir}/" || scp -v "${tarname}" "${destdir}/"
cmd="sha1sum ${destdir}/${tarname} > ${destdir}/${tarname}.sha1sum"
[ ${local} -eq 0 ] && ssh ${desthost} "${cmd}" || eval "${cmd}"
if [ "${bstable}" -eq 1 ]; then
	echo "Updating ${destdir}/latest.tar.gz"
	cmd="(cd ${destdir}; rm -f latest.tar.gz; ln -s ${tarname} latest.tar.gz; ls -l latest.tar.gz ${tarname})"
	[ ${local} -eq 0 ] && ssh ${desthost} "${cmd}" || eval "${cmd}"
fi

echo " 
Installing on gnome.org"
scp "${tarname}" pwieser@master.gnome.org:
ssh pwieser@master.gnome.org install-module -u ${tarname}

echo " 
Installing on kimsufi"
destdir="/home/www/${product}/tarballs"
scp "${tarname}" maintainer@kimsufi:${destdir}/
ssh maintainer@kimsufi "sha1sum ${destdir}/${tarname} > ${destdir}/${tarname}.sha1sum"
if [ ${bstable} -eq 1 ]; then
	echo "Updating ${destdir}/latest.tar.gz"
	ssh maintainer@kimsufi "cd ${destdir}; rm -f latest.tar.gz; ln -s ${tarname} latest.tar.gz; ls -l latest.tar.gz ${tarname}
fi

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
