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

dnl
dnl Managing an other file manager begins here.
dnl ===========================================
dnl 1. update fma_required_versions with the minimal version of the
dnl    file manager
dnl 2. in macro FMA_TARGET_FILE_MANAGER, add a line to call a (new)
dnl    macro _AC_FMA_WITH_<filemanager>
dnl 3. define the new macro _AC_FMA_WITH_<filemanager> below in this
dnl    file
dnl 4. define a new m4/fma-fm-<filemanager>.m4 macro file to check for
dnl    the presence of the extension library/api
dnl 5. update configure.ac for the final output summary
dnl 6. update src/api/fma-fm-defines.h with the adhoc variable types
dnl    and the function names
dnl 7. update src/plugin-menu/Makefile.am
dnl 8. update src/plugin-tracker/Makefile.am
dnl 9. thoroughly test!
dnl

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

	let fma_fm_candidate=0
	let fma_fm_count=0

	_AC_FMA_WITH_NAUTILUS
	_AC_FMA_WITH_NEMO

	if test ${fma_fm_count} -eq 0; then
		_FMA_CHECK_MODULE_MSG([yes],[No suitable target file manager found])
	fi
])

# define a --with<file-manager> option
# (E): 1: name of the option (full lower-case, eg: 'nautilus')
#      2: name of the file manager (may be funny capitalized, eg: 'Nautilus')
# this macro is successively called for Nautilus and Nemo
AC_DEFUN([_AC_FMA_WITHAFM],[
	AC_ARG_WITH([$1],
    	[AS_HELP_STRING([--with-$1],
			[compile plugins for $2 @<:@default=auto@:>@])],
			[],
		[with_$1=auto])
])

# targeting file manager: nautilus
# user may specify --with[out]-nautilus; default is to rely on the 
#  availability of the extensions libraries/apis
# requires: nautilus-devel be installed
# supplementary options: --with-nautilus-extdir
AC_DEFUN([_AC_FMA_WITH_NAUTILUS],[

	_AC_FMA_WITHAFM([nautilus],[Nautilus])

	let fma_fm_candidate+=1
	AC_SUBST([NAUTILUS_ID],[${fma_fm_candidate}])
	AC_SUBST([NAUTILUS_LABEL],[Nautilus])
	AC_DEFINE_UNQUOTED([NAUTILUS_ID],[${fma_fm_candidate}],[Identify the candidate file manager])

	AS_IF([test "$with_nautilus" != "no"],[FMA_CHECK_FOR_NAUTILUS])

	dnl AS_ECHO([with_nautilus_ok=${with_nautilus_ok}])
	if test "${with_nautilus_ok}" = "yes"; then
		let fma_fm_count+=1
	fi

	AM_CONDITIONAL([HAVE_NAUTILUS], [test "${with_nautilus_ok}" = "yes"])
])

# targeting file manager: nemo
# user may specify --with[out]-nemo; default is to rely on the 
#  availability of the extensions libraries/apis
# requires: nemo-devel be installed
# supplementary options: --with-nemo-extdir
AC_DEFUN([_AC_FMA_WITH_NEMO],[

	_AC_FMA_WITHAFM([nemo],[Nemo])

	let fma_fm_candidate+=1
	AC_SUBST([NEMO_ID],[${fma_fm_candidate}])
	AC_SUBST([NEMO_LABEL],[Nemo])
	AC_DEFINE_UNQUOTED([NEMO_ID],[${fma_fm_candidate}],[Identify the candidate file manager])

	AS_IF([test "$with_nemo" != "no"],[FMA_CHECK_FOR_NEMO])

	if test "${with_nemo_ok}" = "yes"; then
		let fma_fm_count+=1
	fi

	AM_CONDITIONAL([HAVE_NEMO], [test "${with_nemo_ok}" = "yes"])
])
