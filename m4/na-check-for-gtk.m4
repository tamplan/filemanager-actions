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

# serial 2 review the macro behavior
#          add version of selected packages

dnl let the user choose the Gtk+ version he wants build against
dnl --with-gtk+=[2|3]
dnl
dnl if the --with-gtk+ option is specified, an argument is required
dnl else, Gtk+-3.0 is first tested, then Gtk+-2.0

AC_DEFUN([NA_CHECK_FOR_GTK],[
	AC_REQUIRE([_AC_NA_ARG_GTK])dnl
	AC_REQUIRE([_AC_NA_CHECK_GTK])dnl
])

AC_DEFUN([_AC_NA_ARG_GTK],[
	AC_ARG_WITH([gtk],
		AC_HELP_STRING(
			[--with-gtk=@<:@2|3@:>@],
			[the Gtk+ version to build against @<:@auto@:>@]),
		[with_gtk=$withval],
		[with_gtk="auto"])
])

AC_DEFUN([_AC_NA_CHECK_GTK],[
	if test "${with_gtk}" = "auto"; then
		_AC_NA_CHECK_FOR_GTK3
		if test "${have_gtk3}" != "yes"; then
			_AC_NA_CHECK_FOR_GTK2
			if test "${have_gtk2}" != "yes"; then
				AC_MSG_WARN([unable to find any suitable Gtk+ development library])
				let na_fatal_count+=1
			fi
		fi
	else
		if test "${with_gtk}" = "2"; then
			_AC_NA_CHECK_FOR_GTK2
			if test "${have_gtk2}" != "yes"; then
				AC_MSG_WARN([unable to build against Gtk+ v2; try to install gtk2-devel package])
				let na_fatal_count+=1
			fi
		else
			if test "${with_gtk}" = "3"; then
				_AC_NA_CHECK_FOR_GTK3
				if test "${have_gtk3}" != "yes"; then
					AC_MSG_WARN([unable to build against Gtk+ v3; try to install gtk3-devel package])
					let na_fatal_count+=1
				fi
			else
				AC_MSG_WARN([--with-gtk=${with_gtk}: invalid argument])
				let na_fatal_count+=1
			fi
		fi
	fi
])

dnl test for Gtk+-3.0 and its dependancies
dnl set have_gtk3=yes if all is ok

AC_DEFUN([_AC_NA_CHECK_FOR_GTK3],[
	PKG_CHECK_MODULES([GTK3],[gtk+-3.0 >= ${gtk_required}],[have_gtk3=yes],[have_gtk3=no])

	if test "${have_gtk3}" = "yes"; then
		NA_CHECK_MODULE([UNIQUE],[unique-3.0],[no])
		if test "${have_UNIQUE}" != "yes"; then
			have_gtk3="no"
		else
			msg_gtk_version=$(pkg-config --modversion gtk+-3.0)
			msg_unique_version=$(pkg-config --modversion unique-3.0)
			NAUTILUS_ACTIONS_CFLAGS="${NAUTILUS_ACTIONS_CFLAGS} ${GTK3_CFLAGS}"
			NAUTILUS_ACTIONS_LIBS="${NAUTILUS_ACTIONS_LIBS} ${GTK3_LIBS}"
		fi
	fi
])

dnl test for Gtk+-2.0 and its dependancies
dnl set have_gtk2=yes if all is ok

AC_DEFUN([_AC_NA_CHECK_FOR_GTK2],[
	PKG_CHECK_MODULES([GTK2],[gtk+-2.0 >= ${gtk_required}],[have_gtk2=yes],[have_gtk2=no])

	if test "${have_gtk2}" = "yes"; then
		NA_CHECK_MODULE([UNIQUE],[unique-1.0],[no])
		if test "${have_UNIQUE}" != "yes"; then
			have_gtk2="no"
		else 
			msg_gtk_version=$(pkg-config --modversion gtk+-2.0)
			msg_unique_version=$(pkg-config --modversion unique-1.0)
			NAUTILUS_ACTIONS_CFLAGS="${NAUTILUS_ACTIONS_CFLAGS} ${GTK2_CFLAGS}"
			NAUTILUS_ACTIONS_LIBS="${NAUTILUS_ACTIONS_LIBS} ${GTK2_LIBS}"
		fi
	fi
])
