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

# serial 3 renamed as FMA_TARGET_FILE_MANAGER

dnl Usage: FMA_TARGET_FILE_MANAGER
dnl Object: choose the target file-manager
dnl Principe: FileManager-Actions plugins must be compiled for targeting
dnl   a specific file manager because each file manager has its own set
dnl   of extensions.
dnl   The macro examines the available installed packages, and compiles
dnl   our plugins for every suitable found API.
dnl   Each file manager may also have specific extensions with may be
dnl   configured via relative macros.
dnl
AC_DEFUN([FMA_TARGET_FILE_MANAGER],[

	let fma_fm_count=0

	AC_REQUIRE([_AC_FMA_WITH_NAUTILUS])
	AC_REQUIRE([_AC_FMA_WITH_NEMO])

	if test ${fma_fm_count} -eq 0; then
		_FMA_CHECK_MODULE_MSG([yes],[No suitable target file manager found])
	fi
])

# targeting file manager: nautilus
# user may specify --with[out]-nautilus; default is to rely on the 
#  availability of the extensions libraries/apis
# requires: nautilus-devel be installed
# supplementary options: --with-nautilus-extdir
AC_DEFUN([_AC_FMA_WITH_NAUTILUS],[

	AC_ARG_WITH([nautilus],
    	[AS_HELP_STRING([--with-nautilus],
			[compile plugins for Nautilus @<:@default=auto@:>@])],
			[],
		[with_nautilus=auto])

	AS_IF([test "$with_nautilus" != "no"],[FMA_CHECK_FOR_NAUTILUS])

	dnl AS_ECHO([with_nautilus_ok=${with_nautilus_ok}])
	if test "${with_nautilus_ok}" = "yes"; then
		let fma_fm_count+=1
		dnl AS_ECHO([fma_fm_count=${fma_fm_count}])
	fi

	AM_CONDITIONAL([HAVE_NAUTILUS], [test "${with_nautilus_ok}" = "yes"])
])

# targeting file manager: nemo
# user may specify --with[out]-nemo; default is to rely on the 
#  availability of the extensions libraries/apis
# requires: nemo-devel be installed
# supplementary options: --with-nemo-extdir
AC_DEFUN([_AC_FMA_WITH_NEMO],[

	AC_ARG_WITH([nemo],
    	[AS_HELP_STRING([--with-nemo],
			[compile plugins for Nemo @<:@default=auto@:>@])],
			[],
		[with_nemo=auto])

	AS_IF([test "$with_nemo" != "no"],[FMA_CHECK_FOR_NEMO])

	if test "${with_nemo_ok}" = "yes"; then
		let fma_fm_count+=1
	fi

	AM_CONDITIONAL([HAVE_NEMO], [test "${with_nemo_ok}" = "yes"])
])
