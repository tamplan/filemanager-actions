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

# serial 1 introduce FMA_CHECK_FOR_CAJA

dnl Usage: FMA_CHECK_FOR_CAJA
dnl Decription: Whether the user has specified '--with-caja' or does
dnl   not have specified anything (while he does not have specified
dnl   --without-caja), we are checking here if Caja is a suitable
dnl   target file manager.
dnl   This requires that the extensions library be installed, and that
dnl   the extensions directory be defined.
dnl
AC_DEFUN([FMA_CHECK_FOR_CAJA],[
	with_caja_ok="no"
	AC_REQUIRE([_FMA_CAJA_EXTDIR])
	if test "${with_caja_extdir}" = ""; then
		AC_MSG_WARN([Unable to determine Caja extension folder, please use --with-caja-extdir option])
	else
		FMA_CHECK_MODULE([CAJA_EXTENSION],[libcaja-extension],[${caja_required}],[yes])
		if test "${have_CAJA_EXTENSION}" = "yes"; then
			with_caja_ok="yes"
		fi
	fi
	if test "${with_caja_ok}" = "yes"; then
		# Check for menu update function
		AC_CHECK_LIB([caja-extension],[caja_menu_item_new],[],[with_caja_ok="no"])
		# doesn't make the two following checks fatal
		AC_CHECK_FUNCS([caja_menu_provider_emit_items_updated_signal])
		#  add toolbar items
		AC_CHECK_FUNCS([caja_menu_provider_get_toolbar_items])
	fi
	if test "${with_caja_ok}" = "yes"; then
		AC_MSG_NOTICE([installing Caja plugins in ${with_caja_extdir}])
		AC_SUBST([CAJA_EXTENSIONS_DIR],[${with_caja_extdir}])
	fi
])

# let the user specify an alternate caja-extension dir
# --with-caja-extdir=<dir>
#
AC_DEFUN([_FMA_CAJA_EXTDIR],[

	AC_ARG_WITH([caja-extdir],
		AC_HELP_STRING(
			[--with-caja-extdir=DIR],
			[Caja extensions directory @<:@auto@:>@]),
		[with_caja_extdir=$withval],
		[with_caja_extdir=""])

	if test "${with_caja_extdir}" = ""; then
		if test "{PKG_CONFIG}" != ""; then
			with_caja_extdir=`${PKG_CONFIG} --variable=extensiondir libcaja-extension`
		fi
	fi
])
