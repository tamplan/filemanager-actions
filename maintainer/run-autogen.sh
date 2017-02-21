#!/bin/sh
# FileManager-Actions
# A file-manager extension which offers configurable context menu actions.
#
# Copyright (C) 2005 The GNOME Foundation
# Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
# Copyright (C) 2009-2015 Pierre Wieser and others (see AUTHORS)
#
# FileManager-Actions is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# FileManager-Actions is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with FileManager-Actions; see the file COPYING. If not, see
# <http://www.gnu.org/licenses/>.
#
# Authors:
#   Frederic Ruaudel <grumz@grumz.net>
#   Rodrigo Moya <rodrigo@gnome-db.org>
#   Pierre Wieser <pwieser@trychlos.org>
#   ... and many others (see AUTHORS)
#
# Rationale
# =========
# 'run-autogen.sh' (this script) is a tool dedicated to the maintainer.
# It mainly gtkdocize-s and autoreconf-es the source tree.
#
# Note that, because 'aclocal' embeds its version number in some
# generated files (and mainly in 'aclocal.m4'), it may be needed to
# execute it from the oldest targeted distribution.
# This was true with Ubuntu 12 LTS, while this appears to be no more
# the case with Debian 7 Wheezy.
#
# As a maintainer shortcut, this 'run-autogen.sh' script also creates
# a convenience 'run-configure.sh' script in the current working
# directory, and executes it. This 'run-configure.sh' script doesn't
# generate the documentation (neither manuals nor reference).

maintainer_dir="$(cd "${0%/*}"; pwd)"
top_srcdir="${maintainer_dir%/*}"
cwd_dir="$(pwd)"

# check that we are able to address to top source tree
if [ ! -f "${top_srcdir}/configure.ac" ]; then
	echo "> Unable to find 'configure.ac' in ${top_srcdir}" 1>&2
	exit 1
fi

PkgName=`autoconf --trace 'AC_INIT:$1' "${top_srcdir}/configure.ac"`
pkgname=$(echo $PkgName | tr '[[:upper:]]' '[[:lower:]]')

 # a filemanager-actions-x.y may remain after an aborted make distcheck
 # such a directory breaks gnome-autogen.sh generation
 # so clean it here
for d in $(find ${top_srcdir} -maxdepth 2 -type d -name "${pkgname}-*"); do
	echo "> Removing $d"
	chmod -R u+w $d
	rm -fr $d
done

# aclocal expects to find 'configure.ac' in its current working directory
# (i.e. to be run from top source tree)
(
 cd "${top_srcdir}"
 echo "> Running aclocal"
 m4_dir=`autoconf --trace 'AC_CONFIG_MACRO_DIR:$1' configure.ac`
 aclocal -I "${m4_dir}" --install || exit 1
)

# requires gtk-doc package
# used for Developer Reference Manual generation (devhelp)
# gtkdocize expects to find 'configure.ac' in its current working directory
# (i.e. to be run from top source tree)
(
 cd "${top_srcdir}"
 echo "> Running gtkdocize"
 gtkdocize --copy || exit 1
)

# autoreconf expects to find 'configure.ac' in its current working directory
# (i.e. to be run from top source tree)
# this notably generate the 'configure' script and all Makefile's.in in
# the source tree
(
 cd "${top_srcdir}"
 echo "> Running autoreconf"
 autoreconf --verbose --force --install -Wno-portability || exit 1
)

# creates the 'run-configure.sh' convenience script in the current
# working directory
runconf="./run-configure.sh"
echo "> Generating ${runconf}"
cat <<EOF >${runconf}
#!/bin/sh
# FileManager-Actions
# A file-manager extension which offers configurable context menu actions.
#
# Copyright (C) 2005 The GNOME Foundation
# Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
# Copyright (C) 2009-2015 Pierre Wieser and others (see AUTHORS)
#
# FileManager-Actions is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# FileManager-Actions is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with FileManager-Actions; see the file COPYING. If not, see
# <http://www.gnu.org/licenses/>.
#
# Authors:
#   Frederic Ruaudel <grumz@grumz.net>
#   Rodrigo Moya <rodrigo@gnome-db.org>
#   Pierre Wieser <pwieser@trychlos.org>
#   ... and many others (see AUTHORS)
#
# WARNING
#   This file has been automatically generated
#   by $(uname -n):$0
#   on $(date) - Please do not manually modify it.

# top_srcdir here is the root of the source directory
#top_srcdir="\$(cd \${0%/*}; pwd)"
top_srcdir="${top_srcdir}"

# heredir is the root of the _build/_install directories
heredir=\$(pwd)

mkdir -p \${heredir}/_build
cd \${heredir}/_build

conf_cmd="\${top_srcdir}/configure"
conf_args=""
conf_args="\${conf_args} --prefix=\${heredir}/_install"
conf_args="\${conf_args} --with-nautilus-extdir=\${heredir}/_install/lib/nautilus"
#conf_args="\${conf_args} --with-nemo-extdir=\${heredir}/_install/lib/nemo"
#conf_args="\${conf_args} --with-caja-extdir=\${heredir}/_install/lib/caja"
conf_args="\${conf_args} --enable-maintainer-mode"
conf_args="\${conf_args} $*"
conf_args="\${conf_args} \$*"

tput bold
echo "\${conf_cmd} \${conf_args}
"
tput sgr0

\${conf_cmd} \${conf_args}
EOF
chmod a+x ${runconf}

echo "> Executing ${runconf}
"
${runconf} &&
make -C _build &&
make -C _build install
