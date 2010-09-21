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

dnl add:
dnl --enable-html-manuals output user manual in HTML, all locales
dnl --enable-pdf-manuals  output user manual in PDF, all locales
dnl --with-db2html        use db2html (docbook-utils) to generate HTML
dnl --with-gdt            use gnome-doc-tool (gnome-doc-utils) to generate HTML (default)
dnl
dnl usage:  NA_ENABLE_MANUALS

AC_DEFUN([NA_ENABLE_MANUALS],[
	AC_REQUIRE([_AC_ARG_NA_ENABLE_HTML_MANUALS])dnl
	AC_REQUIRE([_AC_ARG_NA_WITH_DB2HTML])dnl
	AC_REQUIRE([_AC_ARG_NA_WITH_GDT])dnl
	AC_REQUIRE([_AC_ARG_NA_ENABLE_PDF_MANUALS])dnl
	
	_CHECK_FOR_HTML_MANUALS
	_CHECK_FOR_PDF_MANUALS
])

AC_DEFUN([_AC_ARG_NA_ENABLE_HTML_MANUALS],[
	AC_ARG_ENABLE(
		[html-manuals],
		AC_HELP_STRING(
			[--enable-html-manuals],
			[build HTML manuals @<:@default=no@:>@]),,[enable_html_manuals="no"])
])

AC_DEFUN([_AC_ARG_NA_WITH_DB2HTML],[
	AC_ARG_WITH(
		[db2html],
		AC_HELP_STRING(
			[--with-db2html],
			[use db2html to generate HTML documents @<:@no@:>@]),,[with_db2html="no"])
])

AC_DEFUN([_AC_ARG_NA_WITH_GDT],[
	AC_ARG_WITH(
		[gdt],
		AC_HELP_STRING(
			[--with-gdt],
			[use gnome-doc-tool to generate HTML documents @<:@yes@:>@]),,[with_gdt="yes"])
])

AC_DEFUN([_CHECK_FOR_HTML_MANUALS],[
	AC_MSG_CHECKING([whether to build HTML manuals])
	AC_MSG_RESULT(${enable_html_manuals})

	if test "x${enable_html_manuals}" = "xyes"; then
		if test "x${with_db2html}" = "xyes" -a "x${with_gdt}" = "xyes"; then
			AC_MSG_ERROR([--with-db2html and --with-gdt are mutually incompatible])
		fi
		if test "x${with_db2html}" = "xno" -a "x${with_gdt}" = "xno"; then
			AC_MSG_ERROR([--with-db2html or --with-gdt must be choosen])
		fi
		if test "x${with_db2html}" = "xyes"; then
			AC_CHECK_PROG([_db2html_found],[db2html],[yes],[no])
			if test "x${_db2html_found}" = "xno"; then
				AC_MSG_ERROR([db2html not available and --enable-html-manuals --with-db2html requested])
			fi
		fi
		if test "x${with_gdt}" = "xyes"; then
			AC_CHECK_PROG([_gdt_found],[gnome-doc-tool],[yes],[no])
			if test "x${_gdt_found}" = "xno"; then
				AC_MSG_ERROR([gnome-doc-tools not available and --enable-html-manuals requested])
			fi
		fi
	fi

	AC_SUBST([WITH_DB2HTML],[${with_db2html}])
	AC_SUBST([WITH_GDT],[${with_gdt}])

	AM_CONDITIONAL([ENABLE_HTML_MANUALS], [test "x${enable_html_manuals}" = "xyes"])
])

AC_DEFUN([_AC_ARG_NA_ENABLE_PDF_MANUALS],[
	AC_ARG_ENABLE(
		[pdf-manuals],
		AC_HELP_STRING(
			[--enable-pdf-manuals],
			[build PDF manuals (use dblatex) @<:@default=no@:>@]),,[enable_pdf_manuals="no"])
])

AC_DEFUN([_CHECK_FOR_PDF_MANUALS],[
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
