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

# serial 1 creation

dnl add --enable-html-manual and --enable-pdf-manual configure options
dnl usage:  NA_ENABLE_MANUALS

AC_DEFUN([NA_ENABLE_MANUALS],[
	AC_REQUIRE([_AC_ARG_NA_ENABLE_HTML_MANUALS])dnl
	AC_REQUIRE([_AC_ARG_NA_ENABLE_PDF_MANUALS])dnl
])

AC_DEFUN([_AC_ARG_NA_ENABLE_HTML_MANUALS],[
	AC_ARG_ENABLE(
		[html-manuals],
		AC_HELP_STRING(
			[--enable-html-manuals],
			[use db2html to build HTML manuals @<:@default=no@:>@]),,[enable_html_manuals="no"])

	AC_MSG_CHECKING([whether to build HTML manuals])
	AC_MSG_RESULT(${enable_html_manuals})

	if test "x${enable_html_manuals}" = "xyes"; then
		AC_CHECK_PROG([_db2html_found],[db2html],[yes],[no])
		if test "x${_db2html_found}" = "xno"; then
			AC_MSG_ERROR([db2html not available and --enable-html-manual requested])
		fi
	fi

	AM_CONDITIONAL([ENABLE_HTML_MANUALS], [test "x${enable_html_manuals}" = "xyes"])
])

AC_DEFUN([_AC_ARG_NA_ENABLE_PDF_MANUALS],[
	AC_ARG_ENABLE(
		[pdf-manuals],
		AC_HELP_STRING(
			[--enable-pdf-manuals],
			[use dblatex to build PDF manuals @<:@default=no@:>@]),,[enable_pdf_manuals="no"])

	AC_MSG_CHECKING([whether to build PDF manuals])
	AC_MSG_RESULT(${enable_pdf_manuals})

	if test "x${enable_pdf_manuals}" = "xyes"; then
		AC_CHECK_PROG([_dblatex_found],[dblatex],[yes],[no])
		if test "x${_dblatex_found}" = "xno"; then
			AC_MSG_ERROR([dblatex not available and --enable-pdf-manual requested])
		fi
	fi

	AM_CONDITIONAL([ENABLE_PDF_MANUALS], [test "x${enable_pdf_manuals}" = "xyes"])
])
