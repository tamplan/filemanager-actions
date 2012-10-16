#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

PKG_NAME="nautilus-actions"
REQUIRED_INTLTOOL_VERSION=0.35.5

( test -f $srcdir/configure.ac ) || {
	echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
	echo " top-level $PKG_NAME directory"
	exit 1
}

gtkdocize || exit 1

which gnome-autogen.sh || {
	echo "You need to install gnome-common from the GNOME Git"
	exit 1
}

USE_GNOME2_MACROS=1 . gnome-autogen.sh

# pwi 2012-10-12
# starting with NA 3.2.3, we let the GNOME-DOC-PREPARE do its stuff, but
# get rid of the gnome-doc-utils.make standard file, as we are using our
# own hacked version
# (see full rationale in docs/nact/gnome-doc-utils-na.make)
rm -f gnome-doc-utils.make
