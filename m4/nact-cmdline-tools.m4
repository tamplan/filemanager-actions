# Nautilus Actions
#
# Copyright (C) 2005 The GNOME Foundation
# Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
# Copyright (C) 2009 Pierre Wieser and others (see AUTHORS)
#
# This Program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This Program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public
# License along with this Library; see the file COPYING.  If not,
# write to the Free Software Foundation, Inc., 59 Temple Place,
# Suite 330, Boston, MA 02111-1307, USA.
#
# Authors:
#   Frederic Ruaudel <grumz@grumz.net>
#   Rodrigo Moya <rodrigo@gnome-db.org>
#   Pierre Wieser <pwieser@trychlos.org>
#   ... and many others (see AUTHORS)

# serial 1 creation

# commandline tools are built on user option
# they need gtk+ 2.4, and take interest of glib 2.8
# default is to build them if we have gtk+ v 2.4

AC_DEFUN([NACT_COMMANDLINE_TOOLS],[
	AC_REQUIRE([_AC_ARG_NACT_COMMANDLINE_TOOLS])dnl
	AC_REQUIRE([_AC_CHECK_NACT_COMMANDLINE_TOOLS])dnl

	if test "${_ac_commandline_tools}" = "yes"; then

		AC_SUBST([OPTIONAL_SUBDIR],[${_ac_subdir}])
		AC_DEFINE_UNQUOTED([HAVE_GLIB_2_8],[${_ac_have_glib_2_8}],[Version of the glib-2.0 library is at least 2.8])

		_ac_default_config_path=`eval echo $datadir/${PACKAGE_TARNAME}`
		AC_DEFINE_UNQUOTED([DEFAULT_CONFIG_PATH],["${_ac_default_config_path}"],[Default system configuration path])
		AC_MSG_NOTICE([defining default system configuration path as ${_ac_default_config_path}])
		
		PKG_CHECK_MODULES([NAUTILUS_ACTIONS_UTILS], \
			glib-2.0	>= ${GLIB_REQUIRED}		\
			gtk+-2.0	>= ${GTK_REQUIRED}		\
			gthread-2.0	>= ${GLIB_REQUIRED}		\
			gmodule-2.0	>= ${GLIB_REQUIRED}		\
			gconf-2.0	>= ${GCONF_REQUIRED}	\
			gobject-2.0	>= ${GOBJECT_REQUIRED}	\
			libxml-2.0	>= ${LIBXML_REQUIRED})
		AC_SUBST([NAUTILUS_ACTIONS_UTILS_CFLAGS])
		AC_SUBST([NAUTILUS_ACTIONS_UTILS_LIBS])
	fi
])

AC_DEFUN([_AC_ARG_NACT_COMMANDLINE_TOOLS],[
	AC_ARG_ENABLE(
		[commandline-tool],
		AC_HELP_STRING(
			[--enable-commandline-tool],
			[define if command line tools must be build (do not enable if you have GTK+ <= 2.4) @<:@auto@:>@]
		),
	[ac_enable_nact_commandline_tools=${enableval}],
	[ac_enable_nact_commandline_tools="auto"]
	)
])

AC_DEFUN([_AC_CHECK_NACT_COMMANDLINE_TOOLS],[
	_ac_commandline_tools="yes"
	_ac_have_glib_2_8=0
	${PKG_CONFIG} gtk+-2.0 --max-version=2.4
	if test $? -eq 0; then
		_ac_commandline_tools="no"
		AC_MSG_NOTICE([desactivating commandline tools as Gtk+ is too old])
	else
		if test "${ac_enable_nact_commandline_tools}" = "no"; then
			_ac_commandline_tools="no"
		else
			${PKG_CONFIG} glib-2.0 --atleast-version=2.8
			if test $? -eq 0; then
				_ac_have_glib_2_8=1
				AC_MSG_NOTICE([you have Glib greater than 2.8: fine])
			fi
		fi
	fi
	_ac_subdirs=""
	if test "${_ac_commandline_tools}" = "yes"; then
		_ac_subdir="utils"
	fi
])
