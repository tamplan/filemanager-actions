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

if [ ! -f configure.ac ]; then
	echo "> This script is only meant to be run from the top source directory." 1>&2
	exit 1
fi

maintainer_dir=$(cd ${0%/*}; pwd)
top_srcdir="${maintainer_dir%/*}"

builddir="${top_srcdir}/_build"
installdir="${top_srcdir}/_install"

# a filemanager-actions-x.y may remain after an aborted make distcheck
# such a directory breaks gnome-autogen.sh generation
# so clean it here
for d in $(find ${top_srcdir} -maxdepth 2 -type d -name 'filemanager-actions-*'); do
	chmod -R u+w $d
	rm -fr $d
done

rm -fr ${builddir}
rm -fr ${installdir}
find ${top_srcdir}/docs/nact -type f -name '*.html' -o -name '*.pdf' | xargs rm -f
find ${top_srcdir}/docs/nact \( -type d -o -type l \) -name 'stylesheet-images' -o -name 'admon' | xargs rm -fr

${maintainer_dir}/run-autogen.sh --enable-deprecated --enable-gtk-doc --enable-html-manuals &&
	${maintainer_dir}/check-po.sh -nodummy &&
	${maintainer_dir}/check-headers.sh -nodummy -builddir="${builddir}" &&
	desktop-file-validate ${installdir}/share/applications/nact.desktop &&
	make -C ${builddir} distcheck
