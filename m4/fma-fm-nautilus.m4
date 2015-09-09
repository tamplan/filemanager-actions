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

# serial 3 introduce FMA_CHECK_FOR_NAUTILUS

dnl Usage: FMA_CHECK_FOR_NAUTILUS
dnl Decription: Whether the user has specified '--with-nautilus' or does
dnl   not have specified anything (while he does not have specified
dnl   --without-nautilus), we are checking here if Nautilus is a suitable
dnl   target file manager.
dnl   This requires that the extensions library be installed, and that
dnl   the extensions directory be defined.
dnl
AC_DEFUN([FMA_CHECK_FOR_NAUTILUS],[
	with_nautilus_ok="no"
	AC_REQUIRE([_FMA_NAUTILUS_EXTDIR])
	if test "${with_nautilus_extdir}" = ""; then
		AC_MSG_WARN([Unable to determine Nautilus extension folder, please use --with-nautilus-extdir option])
	else
		FMA_CHECK_MODULE([NAUTILUS_EXTENSION],[libnautilus-extension],[${nautilus_required}],[yes])
		if test "${have_NAUTILUS_EXTENSION}" = "yes"; then
			with_nautilus_ok="yes"
		fi
	fi
	if test "${with_nautilus_ok}" = "yes"; then
		# Check for menu update function
		AC_CHECK_LIB([nautilus-extension],[nautilus_menu_item_new],[],[with_nautilus_ok="no"])
		# doesn't make the two following checks fatal
		AC_CHECK_FUNCS([nautilus_menu_provider_emit_items_updated_signal])
		# starting with 2.91.90, Nautilus no more allows extensions to
		#  add toolbar items
		AC_CHECK_FUNCS([nautilus_menu_provider_get_toolbar_items])
	fi
	if test "${with_nautilus_ok}" = "yes"; then
		AC_MSG_NOTICE([installing Nautilus plugins in ${with_nautilus_extdir}])
		AC_SUBST([NAUTILUS_EXTENSIONS_DIR],[${with_nautilus_extdir}])
	fi
])

# let the user specify an alternate nautilus-extension dir
# --with-nautilus-extdir=<dir>
#
AC_DEFUN([_FMA_NAUTILUS_EXTDIR],[
	
	AC_ARG_WITH([nautilus-extdir],
		AC_HELP_STRING(
			[--with-nautilus-extdir=DIR],
			[Nautilus extensions directory @<:@auto@:>@]),
		[with_nautilus_extdir=$withval],
		[with_nautilus_extdir=""])

	if test "${with_nautilus_extdir}" = ""; then
		if test "{PKG_CONFIG}" != ""; then
			with_nautilus_extdir=`${PKG_CONFIG} --variable=extensiondir libnautilus-extension`
		fi
	fi
])
