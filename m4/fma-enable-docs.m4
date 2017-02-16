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

# serial 5 rename as FMA_ENABLE_DOCS

dnl --enable-docs
dnl   This macro targets the maintainer and enable the generation of al
dnl   documentation stuff.
dnl   This is a shortcut for:
dnl   --enable-deprecated
dnl   --enable-gconf
dnl   --enable-scrollkeeper (omf generation)
dnl   --enable-gtk-doc
dnl   --enable-gtk-doc-html (html reference manual generation)
dnl   --enable-html-manuals (html user's guide manual generation)
dnl   --enable-pdf-manuals (pdf user's guide manual generation)
dnl
dnl usage:  FMA_ENABLE_DOCS

AC_DEFUN([FMA_ENABLE_DOCS],[
	AC_REQUIRE([_AC_ARG_FMA_ENABLE_DOCS])dnl

	enable_deprecated="yes"
	enable_gconf="yes"
	enable_scrollkeeper="yes"
	enable_gtk_doc="yes"
	enable_gtk_doc_html="yes"
	enable_html_manuals="yes"
	enable_pdf_manuals="yes"
])

AC_DEFUN([_AC_ARG_FMA_ENABLE_DOCS],[
	AC_ARG_ENABLE(
		[docs],
		AC_HELP_STRING(
			[--enable-docs@<:@no@:>@],
			[build all documentation @<:@gdt@:>@]),
			[enable_docs=$enableval],
			[enable_docs="no"])
])
