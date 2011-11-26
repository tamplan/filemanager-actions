# Nautilus-Actions
# A Nautilus extension which offers configurable context menu actions.
#
# Copyright (C) 2005 The GNOME Foundation
# Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
# Copyright (C) 2009, 2010, 2011 Pierre Wieser and others (see AUTHORS)
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

AC_INIT([Nautilus-Actions],[3.1.5],[maintainer@nautilus-actions.org],,[http://www.nautilus-actions.org])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
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
	tools/Makefile
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
NA_LINK_AS_NEEDED
NA_GNOME_COMPILE_WARNINGS([],[-ansi -Wno-overlength-strings -Wformat=2])
AC_SUBST([AM_CFLAGS],["${AM_CFLAGS} ${WARN_CFLAGS}"])

# GLib marshaling
AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)

# libtool
AM_PROG_LIBTOOL

# localization
# note that this same version is also required by autogen.sh
IT_PROG_INTLTOOL([0.35.5])
GETTEXT_PACKAGE=${PACKAGE}
AC_SUBST([GETTEXT_PACKAGE])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE],["${GETTEXT_PACKAGE}"],[gettext package])
AM_GLIB_GNU_GETTEXT

# Enable deprecated functions ?
NA_ENABLE_DEPRECATED

# Have and want GConf support ?
NA_CHECK_FOR_GCONF

# note that requiring through pkg-config the version number of the
# nautilus-extension library actually returns the version number of
# nautilus itself (e.g. 2.24.2) instead of the library version info
# (1.1.0) ; for the usual final user, this is very analog to requiring
# a whole gnome version
#
# GLib >= 2.16: GVFS/GIO
# Gtk  >= 2.12: GtkBuilder
#
AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
if test "${PKG_CONFIG}" = "no"; then
	AC_MSG_ERROR([You need to install pkg-config])
fi

glib_required=2.16.0
gtk_required=2.12.0

NA_CHECK_FOR_GTK

dnl aclocal complains if the comma is not just after the square bracket
NA_CHECK_MODULE([DBUS],              [dbus-glib-1])
NA_CHECK_MODULE([GCONF],             [gconf-2.0 >= 2.8.0])
NA_CHECK_MODULE([GIO_UNIX],          [gio-unix-2.0])
NA_CHECK_MODULE([GMODULE],           [gmodule-2.0 >= ${glib_required}])
NA_CHECK_MODULE([GLIB],              [glib-2.0 >= ${glib_required}])
NA_CHECK_MODULE([GTOP],              [libgtop-2.0 >= 2.23.1])
NA_CHECK_MODULE([LIBXML],            [libxml-2.0 >= 2.6])
NA_CHECK_MODULE([SM],                [sm >= 1.0])
NA_CHECK_MODULE([UUID],              [uuid])
NA_CHECK_MODULE([NAUTILUS_EXTENSION],[libnautilus-extension >= 2.16])

AC_SUBST([NAUTILUS_ACTIONS_CFLAGS])
AC_SUBST([NAUTILUS_ACTIONS_LIBS])

# check for gtk-doc
# - starting with N-A 3.1.5, gtk-doc is no more required when one want compile
#   the tarball distribution (i.e. without regenerating reference manuals)
# - starting with gtk-doc 1.15, multiple source dirs are directly handled
#   by gtk-doc.make
# - API documentation should not be built (would be incomplete) when deprecated
#   symbols are not enabled
GTK_DOC_CHECK([1.15],[--flavour no-tmpl])
if test "${enable_gtk_doc}" = "yes" -a "${enable_deprecated}" = "no"; then
	AC_MSG_WARN([API documentation will be incomplete as deprecated symbols are disabled])
fi

# check for gnome-doc-utils
# - starting with N-A 3.1.5, gnome-doc-utils is no more required when one want
#   compile the tarball distribution (i.e. without regenerating users's manuals)
GNOME_DOC_INIT(,,[gdu_cv_have_gdu=no])

# defines log domains when in maintainer mode
NA_LOG_DOMAINS

# add --enable-html-manuals and --enable-pdf-manuals configure options
NA_ENABLE_MANUALS

# add --with-default-io-provider option
NA_SET_DEFAULT_IO_PROVIDER([na-desktop])

# when working in a test environment, nautilus extensions are typically
# installed in a non-standard location ; lets specify this location here
NA_NAUTILUS_EXTDIR

# define NA_MAINTAINER_MODE variable
NA_IS_MAINTAINER_MODE
AC_DEFINE([NAUTILUS_ACTIONS_DEBUG],["NAUTILUS_ACTIONS_DEBUG"],[Debug environment variable])

# Check for menu update function
AC_CHECK_LIB(nautilus-extension, nautilus_menu_item_new)
AC_CHECK_FUNCS([nautilus_menu_provider_emit_items_updated_signal])
# starting with 2.91.90, Nautilus no more allows extensions to add toolbar items
AC_CHECK_FUNCS([nautilus_menu_provider_get_toolbar_items])

# display and keep configuration informations
config_options="`$as_echo "$ac_configure_args" | sed 's/^ //; s/[\\""\`\$]/\\\\&/g'`" 
AC_DEFINE_UNQUOTED([NA_CONFIG_OPTIONS],["$0 ${config_options}"],["Configure options"])

AC_OUTPUT

msg_gtk_doc=`if test "x${enable_gtk_doc}" = "xno"; then echo "disabled"; else echo "enabled"; fi`
msg_gtk_version=""
if test "${have_gtk3}" = "yes"; then msg_gtk_version="Gtk+ 3.0"; 
	elif  test "${have_gtk2}" = "yes"; then msg_gtk_version="Gtk+ 2.0"; fi
$as_echo "
	${PACKAGE_STRING} configuration summary:

	Installation prefix             ${prefix}
	Build system type               ${ac_cv_build}
	Nautilus extensions directory   ${with_nautilus_extdir}
	Gtk+ version                    ${msg_gtk_version}
	Default I/O provider            ${with_default_io_provider}
	Enable deprecated functions     ${enable_deprecated}
	GConf enabled                   ${enable_gconf}
	GConf schemas installation      ${msg_schemas_install}
	Maintainer mode                 ${msg_maintainer_mode}
	API Reference generation        ${msg_gtk_doc}
	HTML User's Manuals generation  ${msg_html_manuals}
	PDF User's Manuals generation   ${msg_pdf_manuals}
"
#	Preprocessor flags              ${AM_CPPFLAGS}
#	Compiler flags                  ${AM_CFLAGS} ${NAUTILUS_ACTIONS_CFLAGS}
#	Linker flags                    ${NAUTILUS_ACTIONS_LIBS}
