#!/bin/sh
exec ./autogen.sh \
	--prefix=$(pwd)/install \
	--with-nautilus-extdir=$(pwd)/install/lib/nautilus \
	--disable-schemas-install \
	--enable-gtk-doc \
	--disable-scrollkeeper \
	--enable-html-manuals \
	--with-db2html \
	--without-gdt \
	--enable-pdf-manuals
