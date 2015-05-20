# Nautilus-Actions
# A Nautilus extension which offers configurable context menu actions.
#
# Copyright (C) 2005 The GNOME Foundation
# Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
# Copyright (C) 2009-2014 Pierre Wieser and others (see AUTHORS)
#
# Nautilus-Actions is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# Nautilus-Actions is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Nautilus-Actions; see the file COPYING. If not, see
# <http://www.gnu.org/licenses/>.
#
# Authors:
#   Frederic Ruaudel <grumz@grumz.net>
#   Rodrigo Moya <rodrigo@gnome-db.org>
#   Pierre Wieser <pwieser@trychlos.org>
#   ... and many others (see AUTHORS)

# serial 1 let the user choose a target file-manager
# serial 2 manage Nemo

dnl defaults to nautilus

AC_DEFUN([NA_TARGET_FILE_MANAGER],[

	AC_ARG_ENABLE([file-manager],
		AC_HELP_STRING(
			[--enable-file-manager=@<:@nautilus|nemo@:>@],
			[the targeted file manager @<:@nautilus@:>@]),
		[enable_file_manager=$withval],
		[enable_file_manager="nautilus"])

	if test "${enable_file_manager}" = "nautilus"; then
		AC_MSG_NOTICE([targeting Nautilus file-manager])
		AC_REQUIRE([_AC_NA_FILE_MANAGER_NAUTILUS])dnl

	elif test "${enable_file_manager}" = "nemo"; then
		AC_MSG_NOTICE([targeting Nemo file-manager])
		AC_REQUIRE([_AC_NA_FILE_MANAGER_NEMO])dnl
	fi
])

# target file manager: nautilus
# when working in a test environment, nautilus extensions are typically
# installed in a non-standard location; lets specify this location here
# --with-nautilus-extdir=<dir>

AC_DEFUN([_AC_NA_FILE_MANAGER_NAUTILUS],[

	AC_ARG_WITH(
		[nautilus-extdir],
		AC_HELP_STRING(
			[--with-nautilus-extdir=DIR],
			[nautilus plugins extension directory @<:@auto@:>@]),
		[with_nautilus_extdir=$withval],
		[with_nautilus_extdir=""])

	if test "${with_nautilus_extdir}" = ""; then
		if test "{PKG_CONFIG}" != ""; then
			with_nautilus_extdir=`${PKG_CONFIG} --variable=extensiondir libnautilus-extension`
		fi
	fi
	if test "${with_nautilus_extdir}" = ""; then
		AC_MSG_ERROR([Unable to determine nautilus extension folder, please use --with-nautilus-extdir option])
	else
		AC_MSG_NOTICE([installing plugins in ${with_nautilus_extdir}])
		AC_SUBST([NAUTILUS_EXTENSIONS_DIR],[${with_nautilus_extdir}])
		AC_DEFINE_UNQUOTED([NA_NAUTILUS_EXTENSIONS_DIR],[${with_nautilus_extdir}],[Nautilus extensions directory])
	fi

	NA_CHECK_MODULE([NAUTILUS_EXTENSION],[libnautilus-extension],[${nautilus_required}])

	# Check for menu update function
	AC_CHECK_LIB([nautilus-extension],[nautilus_menu_item_new])
	AC_CHECK_FUNCS([nautilus_menu_provider_emit_items_updated_signal])

	# starting with 2.91.90, Nautilus no more allows extensions to
	#  add toolbar items
	AC_CHECK_FUNCS([nautilus_menu_provider_get_toolbar_items])
])

# target file manager: nemo
# when working in a test environment, nemo extensions are typically
# installed in a non-standard location; lets specify this location here
# --with-nemo-extdir=<dir>

AC_DEFUN([_AC_NA_FILE_MANAGER_NEMO],[

	AC_ARG_WITH(
		[nemo-extdir],
		AC_HELP_STRING(
			[--with-nemo-extdir=DIR],
			[nemo plugins extension directory @<:@auto@:>@]),
		[with_nemo_extdir=$withval],
		[with_nemo_extdir=""])

	if test "${with_nemo_extdir}" = ""; then
		if test "{PKG_CONFIG}" != ""; then
			with_nemo_extdir=`${PKG_CONFIG} --variable=extensiondir libnemo-extension`
		fi
	fi
	if test "${with_nemo_extdir}" = ""; then
		AC_MSG_ERROR([Unable to determine nemo extension folder, please use --with-nemo-extdir option])
	else
		AC_MSG_NOTICE([installing plugins in ${with_nemo_extdir}])
		AC_SUBST([NEMO_EXTENSIONS_DIR],[${with_nemo_extdir}])
		AC_DEFINE_UNQUOTED([NA_NEMO_EXTENSIONS_DIR],[${with_nemo_extdir}],[Nemo extensions directory])
	fi

	NA_CHECK_MODULE([NEMO_EXTENSION],[libnemo-extension],[${nemo_required}])

	# Check for menu update function
	AC_CHECK_LIB([nemo-extension],[nemo_menu_item_new])
	AC_CHECK_FUNCS([nemo_menu_provider_emit_items_updated_signal])
])
