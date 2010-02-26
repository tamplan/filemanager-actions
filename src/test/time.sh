#!/bin/sh
#
# this small script is used both to follow the increase of the product
# and to bench our workstations
# very simple indeed !
#
# should be run from top_srcdir

if [ ! -r ./autogen.sh ]; then
	echo "This script should be ran from top_srcdir." 1>&2
	exit 1
fi

function do_build
{
	./autogen.sh \
		--prefix=$(pwd)/install \
		--with-nautilus-extdir=$(pwd)/install/lib/nautilus \
		--disable-schemas-install							&&
	make clean												&& 
	make													&&
	make install
}

function loop_build
{
	i=0
	while [ ${i} -lt ${count} ]; do
		do_build
		let i+=1
	done
}

###
### MAIN
###

count=1
time loop_build
