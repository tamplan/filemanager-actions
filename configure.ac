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
#   and many others (see AUTHORS)
#
# pwi 2009-05-16 created from configure.in

AC_PREREQ([2.53])

AC_INIT([nautilus-actions],[1.10],[maintainer@nautilus-actions.org])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE

AC_CONFIG_SRCDIR([plugin/nautilus-actions.c])

AC_CONFIG_HEADERS([config.h])

AC_CONFIG_FILES([
	Makefile
	libnautilus-actions/Makefile
	plugin/Makefile
	nact/Makefile
	utils/Makefile
	icons/Makefile
	icons/16x16/Makefile
	icons/22x22/Makefile
	icons/32x32/Makefile
	icons/48x48/Makefile
	icons/scalable/Makefile
	po/Makefile.in
])

#AC_SUBST(ACLOCAL_AMFLAGS, "$ACLOCAL_FLAGS")

# don't agree with maintainer mode use
# see http://www.gnu.org/software/automake/manual/automake.html#index-AM_005fMAINTAINER_005fMODE-1001
# but gnome-autogen.sh forces its usage and gnome_common_init requires it
AM_MAINTAINER_MODE

# check for compiler characteristics and options
AC_PROG_CC
AC_PROG_GCC_TRADITIONAL
AC_STDC_HEADERS

# why ?
#AM_DISABLE_STATIC

# other traditional tools
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Gnome stuff
GNOME_COMMON_INIT
GNOME_MAINTAINER_MODE_DEFINES
GNOME_COMPILE_WARNINGS
AC_SUBST([AM_CPPFLAGS],["${AM_CPPFLAGS} ${DISABLE_DEPRECATED}"])
AC_SUBST([AM_CFLAGS],["${AM_CFLAGS} ${WARN_CFLAGS}"])

# libtool
AM_PROG_LIBTOOL

# localization
# note that this version is also required for in autogen.sh
IT_PROG_INTLTOOL([0.35.5])
GETTEXT_PACKAGE=${PACKAGE}
AC_SUBST([GETTEXT_PACKAGE])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE],["${GETTEXT_PACKAGE}"],[gettext package])
AM_GLIB_GNU_GETTEXT

# GConf support
AC_PATH_PROG([GCONFTOOL],[gconftool-2])
AM_GCONF_SOURCE_2

AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
if test "x$PKG_CONFIG" = "xno"; then
        AC_MSG_ERROR([You need to install pkg-config])
fi

# these are the version as distributed with CentOS 4.x
#
# note that requiring through pkg-config the version number of the
# nautilus-extension library actually returns the version number of
# nautilus itself (e.g. 2.24.2) instead of the library version info
# (1.1.0) ; for the usual final user, this is very analog to requiring
# a whole gnome version
GLIB_REQUIRED=2.4.0
GTK_REQUIRED=2.4.0
GLADE_REQUIRED=2.4.0
GNOME_REQUIRED=2.7.0
NAUTILUS_EXTENSION=2.8.0
GCONF_REQUIRED=2.8.0
LIBXML_REQUIRED=2.6.0
GOBJECT_REQUIRED=2.4.0

#AC_SUBST(GLIB_REQUIRED)
#AC_SUBST(GTK_REQUIRED)
#AC_SUBST(GLADE_REQUIRED)

PKG_CHECK_MODULES([NAUTILUS_ACTIONS], \
	glib-2.0				>= ${GLIB_REQUIRED}			\
	gthread-2.0				>= ${GLIB_REQUIRED}			\
	gmodule-2.0				>= ${GLIB_REQUIRED}			\
	gtk+-2.0				>= ${GTK_REQUIRED}			\
	libglade-2.0			>= ${GLADE_REQUIRED}		\
	libgnome-2.0			>= ${GNOME_REQUIRED}		\
	libgnomeui-2.0			>= ${GNOME_REQUIRED}		\
	gconf-2.0				>= ${GCONF_REQUIRED}		\
	libnautilus-extension	>= ${NAUTILUS_EXTENSION}	\
	uuid)
AC_SUBST([NAUTILUS_ACTIONS_CFLAGS])
AC_SUBST([NAUTILUS_ACTIONS_LIBS])

AC_ARG_WITH(nautilus-extdir,
[ AC_HELP_STRING([--with-nautilus-extdir=EXTDIR],
					  [EXTDIR define the folder where nautilus stores its extentions plugins. [default=auto]])],
[ NAUTILUS_EXTENSIONS_DIR=$withval ],
[ NAUTILUS_EXTENSIONS_DIR=`$PKG_CONFIG --variable=extensiondir libnautilus-extension` ])

AC_SUBST(NAUTILUS_EXTENSIONS_DIR)

if test -z ${NAUTILUS_EXTENSIONS_DIR}; then
	AC_MSG_ERROR([*** Can't determine nautilus extension folder, please use --with-nautilus-extdir options to define it manually ]);
fi

OPTIONAL_SUBDIR="utils"
BUILD_COMMANDLINE_TOOL="yes"
AC_ARG_ENABLE(commandline-tool,
[  --enable-commandline-tool            define if command line tools must be build (disable it if you have GTK+ <= 2.4).
                           [default=auto]],
[ BUILD_COMMANDLINE_TOOL=$enableval ])
if test "x$BUILD_COMMANDLINE_TOOL" = "xno"; then
   OPTIONAL_SUBDIR=""
fi
$PKG_CONFIG glib-2.0 --max-version=2.4
if test "x$?" = "x0"; then
	OPTIONAL_SUBDIR=""
fi
AC_SUBST(OPTIONAL_SUBDIR)

default_config_path=`eval echo $datadir/nautilus-actions`
AC_DEFINE_UNQUOTED(DEFAULT_CONFIG_PATH, "$default_config_path", [Default system configuration path])
$PKG_CONFIG glib-2.0 --atleast-version=2.8
if test "x$?" = "x0"; then
	AC_DEFINE_UNQUOTED(HAVE_GLIB_2_8, 1, [Version of the glib-2.0 library is at least 2.8])
fi

PKG_CHECK_MODULES(NAUTILUS_ACTIONS_UTILS,\
	 glib-2.0 >= $GLIB_REQUIRED                \
	 gthread-2.0 >= $GLIB_REQUIRED             \
	 gmodule-2.0 >= $GLIB_REQUIRED		  \
	 gconf-2.0 >= $GCONF_REQUIRED \
	 gobject-2.0 >= $GOBJECT_REQUIRED \
	 libxml-2.0 >= $LIBXML_REQUIRED)
AC_SUBST(NAUTILUS_ACTIONS_UTILS_CFLAGS)
AC_SUBST(NAUTILUS_ACTIONS_UTILS_LIBS)

# Check for menu update function
AC_CHECK_LIB(nautilus-extension, nautilus_menu_item_new)
AC_CHECK_FUNCS(nautilus_menu_provider_emit_items_updated_signal)

#if test "x${exec_prefix}" = "xNONE"; then
#	if test "x${prefix}" = "xNONE"; then
#		if test "x${bindir}" = "xNONE"; then
#			BINDIR=`eval echo $ac_default_prefix/bin`
#		else
#			BINDIR=`eval echo $bindir`
#		fi
#	else
#		BINDIR=`eval echo $prefix/bin`
#	fi
#else
#	BINDIR=`eval echo $bindir`
#fi
#AC_SUBST(BINDIR)
#	
#if test "x${prefix}" = "xNONE"; then
#  DATADIR="${ac_default_prefix}/${DATADIRNAME}" 
#  LOCALEDIR="${DATADIR}/locale"
#  AC_DEFINE_UNQUOTED(LOCALEDIR, "${LOCALEDIR}", [Locale dir])
#else
#  DATADIR="${prefix}/${DATADIRNAME}"
#  LOCALEDIR="${DATADIR}/locale"
#  AC_DEFINE_UNQUOTED(LOCALEDIR, "${LOCALEDIR}", [Locale dir])
#fi
#AC_SUBST(DATADIR)
#AC_SUBST(LOCALEDIR)

AC_DEFINE_UNQUOTED(NAUTILUS_ACTIONS_CONFIG_VERSION, "2.0", [Version of the configuration format])
AC_DEFINE_UNQUOTED(NAUTILUS_ACTIONS_CONFIG_GCONF_BASEDIR, "/apps/nautilus-actions", [GConf base dir where all config will be stored])

AC_OUTPUT