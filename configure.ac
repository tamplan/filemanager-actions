# Nautilus Actions
# A Nautilus extension which offers configurable context menu actions.
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

AC_PREREQ([2.53])

# 1.11 odd version is unstable
AC_INIT([Nautilus-Actions],[1.11.2],[maintainer@nautilus-actions.org])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE

AC_CONFIG_SRCDIR([src/plugin/nautilus-module.c])

AC_CONFIG_HEADERS([src/config.h])

AC_CONFIG_FILES([
	Makefile
	m4/Makefile
	icons/Makefile
	icons/16x16/Makefile
	icons/22x22/Makefile
	icons/32x32/Makefile
	icons/48x48/Makefile
	icons/scalable/Makefile
	src/Makefile
	src/common/Makefile
	src/nact/Makefile
	src/plugin/Makefile
	src/test/Makefile
	src/utils/Makefile
	po/Makefile.in
])

AC_CONFIG_MACRO_DIR([m4])

# pwi 2009-05-15 disabled as GNOME_COMMON_INIT takes care of that
#AC_SUBST(ACLOCAL_AMFLAGS, "$ACLOCAL_FLAGS")

# don't agree with maintainer mode use
# see http://www.gnu.org/software/automake/manual/automake.html#index-AM_005fMAINTAINER_005fMODE-1001
# but gnome-autogen.sh forces its usage and gnome_common_init requires it
AM_MAINTAINER_MODE

# check for compiler characteristics and options
AC_PROG_CC
AC_PROG_GCC_TRADITIONAL
AC_STDC_HEADERS
AM_DISABLE_STATIC

# other traditional tools
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Gnome stuff
GNOME_COMMON_INIT
GNOME_MAINTAINER_MODE_DEFINES
NA_GNOME_COMPILE_WARNINGS([error],[-ansi])
AC_SUBST([AM_CPPFLAGS],["${AM_CPPFLAGS} ${DISABLE_DEPRECATED}"])
# unique 1.0.8 doesn't satisfy this
#AC_SUBST([AM_CPPFLAGS],["${AM_CPPFLAGS} \
#	-DG_DISABLE_SINGLE_INCLUDES -DGDK_PIXBUF_DISABLE_SINGLE_INCLUDES -DGTK_DISABLE_SINGLE_INCLUDES"])
AC_SUBST([AM_CFLAGS],["${AM_CFLAGS} ${WARN_CFLAGS}"])

# libtool
AM_PROG_LIBTOOL

# localization
# note that this same version is also required for in autogen.sh
IT_PROG_INTLTOOL([0.35.5])
GETTEXT_PACKAGE=${PACKAGE}
AC_SUBST([GETTEXT_PACKAGE])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE],["${GETTEXT_PACKAGE}"],[gettext package])
AM_GLIB_GNU_GETTEXT

# GConf support
AC_PATH_PROG([GCONFTOOL],[gconftool-2])
AM_GCONF_SOURCE_2

# note that requiring through pkg-config the version number of the
# nautilus-extension library actually returns the version number of
# nautilus itself (e.g. 2.24.2) instead of the library version info
# (1.1.0) ; for the usual final user, this is very analog to requiring
# a whole gnome version
#
# Gtk >= 2.12: GtkBuilder
#
GLIB_REQUIRED=2.16.0
GTK_REQUIRED=2.12.0
NAUTILUS_EXTENSION=2.8.0
GCONF_REQUIRED=2.8.0
LIBXML_REQUIRED=2.6.0
GOBJECT_REQUIRED=2.4.0

AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
if test "${PKG_CONFIG}" = "no"; then
	AC_MSG_ERROR([You need to install pkg-config])
fi

PKG_CHECK_MODULES([NAUTILUS_ACTIONS], \
	glib-2.0				>= ${GLIB_REQUIRED}			\
	gthread-2.0				>= ${GLIB_REQUIRED}			\
	gmodule-2.0				>= ${GLIB_REQUIRED}			\
	gtk+-2.0				>= ${GTK_REQUIRED}			\
	gconf-2.0				>= ${GCONF_REQUIRED}		\
	libxml-2.0				>= ${LIBXML_REQUIRED}		\
	libnautilus-extension	>= ${NAUTILUS_EXTENSION}	\
	uuid												\
	unique-1.0
)
AC_SUBST([NAUTILUS_ACTIONS_CFLAGS])
AC_SUBST([NAUTILUS_ACTIONS_LIBS])

# defines log domains when in maintainer mode
NA_LOG_DOMAINS

# when working in a test environment, nautilus is typically installed in
# a non-standard location ; lets specify this here
NA_NAUTILUS_EXTDIR

# building or not the commandline tools
NA_COMMANDLINE_TOOLS

# define NACT_MAINTAINER_MODE variable
NA_IS_MAINTAINER_MODE

# Check for menu update function
AC_CHECK_LIB(nautilus-extension, nautilus_menu_item_new)
AC_CHECK_FUNCS(nautilus_menu_provider_emit_items_updated_signal)

AC_DEFINE_UNQUOTED(NAUTILUS_ACTIONS_CONFIG_VERSION, "2.0", [Version of the configuration format])
AC_DEFINE_UNQUOTED(NAUTILUS_ACTIONS_CONFIG_GCONF_BASEDIR, "/apps/nautilus-actions", [GConf base dir where all config will be stored])

AC_OUTPUT
