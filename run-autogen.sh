#!/bin/sh
#
# pwi 2011-11-28 goal is to have a single source tree, being able to easily
#                build it in several virtual guests which all have a read
#                access to this source tree
# -> run-distcheck.sh and run-autogen.sh are only executed on the development
#    boxes, while run-configure is meant to build from anywhere
# -> on the development box, _build and _install are subdirectories of the
#    source tree
# -> on the virtual guests, _build and _install are subdirectories of the 
#    current working directory

target=${target:-normal}
srcdir=$(cd ${0%/*}; pwd)

# a nautilus-actions-x.y may remain after an aborted make distcheck
# such a directory breaks gnome-autogen.sh generation
# so clean it here
for d in $(find ${srcdir} -maxdepth 1 -type d -name 'nautilus-actions-*'); do
	chmod -R u+w $d
	rm -fr $d
done

conf_args=""

if [ "${target}" = "normal" ]; then
	conf_args="${conf_args} --disable-deprecated"
	conf_args="${conf_args} --disable-gtk-doc"
	conf_args="${conf_args} --disable-html-manuals"
	conf_args="${conf_args} --disable-pdf-manuals"

# 'doc' mode: enable deprecated, manuals and gtk-doc
elif [ "${target}" = "doc" ]; then
	conf_args="${conf_args} --enable-deprecated"
	conf_args="${conf_args} --enable-gtk-doc"
	conf_args="${conf_args} --enable-gtk-doc-html"
	conf_args="${conf_args} --enable-gtk-doc-pdf"
	conf_args="${conf_args} --enable-html-manuals"
	conf_args="${conf_args} --enable-pdf-manuals"
fi

# Build with Gtk+ 3 (actually a 2.97.x unstable version)
# installed in ~/.local/jhbuild
#
# Note that building with Gtk 3.0 not only requires that we have a
# Gtk+ 3 available library, but also that all our required libraries
# only depend of Gtk+ 3.0. In our case, we have:
# $ grep gtk+-2.0 /usr/lib/pkgconfig/*
#   libnautilus-extension.pc:Requires: glib-2.0 gio-2.0 gtk+-2.0
#   unique-1.0.pc:Requires: gtk+-2.0
#
#[ "${target}" = "jhbuild" ] &&
#	export autogen_prefix=${HOME}/data/jhbuild/run &&
#	PKG_CONFIG_PATH=${autogen_prefix}/lib/pkgconfig \
#	LD_LIBRARY_PATH=${autogen_prefix}/lib \
#		exec ./autogen.sh \
#			--prefix=${autogen_prefix} \
#			--sysconfdir=/etc \
#			--disable-schemas-install \
#			$*

NOCONFIGURE=1 ${srcdir}/autogen.sh

runconf=${srcdir}/run-configure.sh
echo "
Generating ${runconf}"

cat <<EOF >${runconf}
#!/bin/sh

# srcdir here is the root of the source directory
target=\${target:-normal}
srcdir=\$(cd \${0%/*}; pwd)

# heredir is the root of the _build/_install directories
heredir=\$(pwd)

mkdir -p \${heredir}/_build
cd \${heredir}/_build

conf_cmd="\${srcdir}/configure"
conf_args="${conf_args}"
conf_args="\${conf_args} --prefix=\${heredir}/_install"
conf_args="\${conf_args} --sysconfdir=/etc"
conf_args="\${conf_args} --with-nautilus-extdir=\${heredir}/_install/lib/nautilus"
conf_args="\${conf_args} --disable-schemas-install"
conf_args="\${conf_args} --disable-scrollkeeper"
conf_args="\${conf_args} --enable-maintainer-mode"
conf_args="\${conf_args} \$*"

tput bold
echo "\${conf_cmd} \${conf_args}
"
tput sgr0

\${conf_cmd} \${conf_args}
EOF

echo "Executing ${runconf}
"
chmod a+x ${runconf}
${runconf} &&
make -C _build &&
make -C _build install
