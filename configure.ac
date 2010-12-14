# Nautilus Actions
# A Nautilus extension which offers configurable context menu actions.
#
# Copyright (C) 2005 The GNOME Foundation
# Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
# Copyright (C) 2009, 2010 Pierre Wieser and others (see AUTHORS)
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

AC_INIT([Nautilus-Actions],[3.0.3],[maintainer@nautilus-actions.org],,[http://www.nautilus-actions.org])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES])

AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE

AC_CONFIG_SRCDIR([src/plugin-menu/nautilus-module.c])

AC_CONFIG_HEADERS([src/config.h])

AC_CONFIG_FILES([
	Makefile
	data/Makefile
	data/actions/Makefile
	data/exports/Makefile
	data/gconf-schemas/Makefile
	docs/Makefile
	docs/nact/Makefile
	docs/reference/Makefile
	docs/reference/version.xml
	m4/Makefile
	icons/Makefile
	icons/16x16/Makefile
	icons/22x22/Makefile
	icons/32x32/Makefile
	icons/48x48/Makefile
	icons/scalable/Makefile
	src/Makefile
	src/api/Makefile
	src/core/Makefile
	src/io-desktop/Makefile
	src/io-gconf/Makefile
	src/io-xml/Makefile
	src/plugin-menu/Makefile
	src/plugin-tracker/Makefile
	src/nact/Makefile
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
GNOME_DOC_INIT
GNOME_MAINTAINER_MODE_DEFINES
NA_GNOME_COMPILE_WARNINGS([],[-ansi -Wno-overlength-strings -Wformat=2])
AC_SUBST([AM_CFLAGS],["${AM_CFLAGS} ${WARN_CFLAGS}"])

# GLib marshaling
AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)

# check for gtk-doc
GTK_DOC_CHECK([1.10],[--flavour no-tmpl])

# libtool
AM_PROG_LIBTOOL

# localization
# note that this same version is also required by autogen.sh
IT_PROG_INTLTOOL([0.35.5])
GETTEXT_PACKAGE=${PACKAGE}
AC_SUBST([GETTEXT_PACKAGE])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE],["${GETTEXT_PACKAGE}"],[gettext package])
AM_GLIB_GNU_GETTEXT

# GConf support
AC_PATH_PROG([GCONFTOOL],[gconftool-2],[no])
AM_GCONF_SOURCE_2

# note that requiring through pkg-config the version number of the
# nautilus-extension library actually returns the version number of
# nautilus itself (e.g. 2.24.2) instead of the library version info
# (1.1.0) ; for the usual final user, this is very analog to requiring
# a whole gnome version
#
# GLib >= 2.16: GVFS/GIO
# Gtk  >= 2.12: GtkBuilder
#
# [configure.ac, nautilus-actions.c] remove test for
# 	nautilus_menu_provider_emit_items_updated_signal() when Gnome >= 2.16
#
GLIB_REQUIRED=2.16.0
GTK_REQUIRED=2.12.0
NAUTILUS_EXTENSION_REQUIRED=2.8.0
GCONF_REQUIRED=2.8.0
LIBXML_REQUIRED=2.6.0
GOBJECT_REQUIRED=2.4.0
LIBGTOP_REQUIRED=2.23.1

AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
if test "${PKG_CONFIG}" = "no"; then
	AC_MSG_ERROR([You need to install pkg-config])
fi

PKG_CHECK_MODULES([DBUS],				[dbus-glib-1])
PKG_CHECK_MODULES([GCONF],				[gconf-2.0		>= ${GCONF_REQUIRED}])
PKG_CHECK_MODULES([GMODULE],			[gmodule-2.0	>= ${GLIB_REQUIRED}])
PKG_CHECK_MODULES([GLIB],				[glib-2.0		>= ${GLIB_REQUIRED}])
PKG_CHECK_MODULES([GTK],				[gtk+-2.0		>= ${GTK_REQUIRED}])
PKG_CHECK_MODULES([GTOP],				[libgtop-2.0	>= ${LIBGTOP_REQUIRED}])
PKG_CHECK_MODULES([LIBXML],				[libxml-2.0		>= ${LIBXML_REQUIRED}])
PKG_CHECK_MODULES([SM],					[sm				>= 1.0.0])
PKG_CHECK_MODULES([UNIQUE],				[unique-1.0])
PKG_CHECK_MODULES([UUID],				[uuid])
PKG_CHECK_MODULES([NAUTILUS_EXTENSION],	[libnautilus-extension >= ${NAUTILUS_EXTENSION_REQUIRED}])

AC_SUBST([NAUTILUS_ACTIONS_CFLAGS],["	\
	${DBUS_CFLAGS}						\
	${GCONF_CFLAGS}						\
	${CGMODULE_CFLAGS}					\
	${GLIB_CFLAGS}						\
	${GTK_CFLAGS}						\
	${GTOP_CFLAGS}						\
	${LIBXML_CFLAGS}					\
	${SM_CFLAGS}						\
	${UNIQUE_CFLAGS}					\
	${UUID_CFLAGS}						\
	${NAUTILUS_EXTENSION_CFLAGS}		\
"])
AC_DEFINE_UNQUOTED([NAUTILUS_ACTIONS_CFLAGS],["${NAUTILUS_ACTIONS_CFLAGS}"],[N-A compiler flags])

AC_SUBST([NAUTILUS_ACTIONS_LIBS],["		\
	${DBUS_LIBS}						\
	${GCONF_LIBS}						\
	${CGMODULE_LIBS}					\
	${GLIB_LIBS}						\
	${GTK_LIBS}							\
	${GTOP_LIBS}						\
	${LIBXML_LIBS}						\
	${SM_LIBS}							\
	${UNIQUE_LIBS}						\
	${UUID_LIBS}						\
	${NAUTILUS_EXTENSION_LIBS}			\
"])
AC_DEFINE_UNQUOTED([NAUTILUS_ACTIONS_LIBS],["${NAUTILUS_ACTIONS_LIBS}"],[N-A linker flags])

# defines log domains when in maintainer mode
NA_LOG_DOMAINS

# add --enable-html-manuals and --enable-pdf-manuals configure options
NA_ENABLE_MANUALS

# add --with-default-io-provider option
NA_SET_DEFAULT_IO_PROVIDER([na-desktop])

# when working in a test environment, nautilus extensions are typically
# installed in a non-standard location ; lets specify them here
NA_NAUTILUS_EXTDIR

# define NA_MAINTAINER_MODE variable
NA_IS_MAINTAINER_MODE
AC_DEFINE([NAUTILUS_ACTIONS_DEBUG],["NAUTILUS_ACTIONS_DEBUG"],[Debug environment variable])

# Check for menu update function
AC_CHECK_LIB(nautilus-extension, nautilus_menu_item_new)
AC_CHECK_FUNCS(nautilus_menu_provider_emit_items_updated_signal)

# display and keep configuration informations
config_options="`$as_echo "$ac_configure_args" | sed 's/^ //; s/[\\""\`\$]/\\\\&/g'`" 
AC_DEFINE_UNQUOTED([NA_CONFIG_OPTIONS],["$0 ${config_options}"],["Configure options"])

AC_OUTPUT

if test "x${enable_schemas_install}" = "xno"; then
	_ac_na_schemas_install="disabled"; else
	_ac_na_schemas_install="enabled in ${GCONF_SCHEMA_FILE_DIR}"
fi
_ac_na_gtk_html_doc=`if test "x${enable_gtk_doc_html}" = "xno"; then echo disabled; else echo "enabled with gtk-doc"; fi`
_ac_na_gtk_pdf_doc=`if test "x${enable_gtk_doc_pdf}" = "xno"; then echo disabled; else echo enabled; fi`
$as_echo "
	${PACKAGE_STRING} configuration summary:

	Installation prefix             ${prefix}
	Build system type               ${ac_cv_build}
	Nautilus extensions directory   ${ac_with_nact_nautilus_extdir}
	Default I/O provider            ${with_default_io_provider}
	GConf schemas installation      ${_ac_na_schemas_install}
	Maintainer mode                 ${_ac_na_maintainer_mode}
	HTML API Reference generation   ${_ac_na_gtk_html_doc}
	PDF API Reference generation    ${_ac_na_gtk_pdf_doc}
	HTML manuals generation         ${_ac_na_html_manuals}
	PDF manuals generation          ${_ac_na_pdf_manuals}
"
#	Preprocessor flags              ${AM_CPPFLAGS}
#	Compiler flags                  ${AM_CFLAGS} ${NAUTILUS_ACTIONS_CFLAGS}
#	Linker flags                    ${NAUTILUS_ACTIONS_LIBS}
