#!/bin/sh

# a nautilus-actions-x.y may remain after an aborted make distcheck
# such a directory breaks gnome-autogen.sh generation
# so clean it here
find . -maxdepth 1 -type d -name 'nautilus-actions-*' | xargs rm -fr

exec ./autogen.sh \
	--prefix=$(pwd)/install \
	--with-nautilus-extdir=$(pwd)/install/lib/nautilus \
	--disable-schemas-install \
	--enable-gtk-doc \
	--enable-gtk-doc-pdf \
	--disable-scrollkeeper \
	--enable-html-manuals \
	--enable-pdf-manuals
