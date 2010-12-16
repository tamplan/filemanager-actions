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

dnl usage:  NA_CHECK_MODULE([var],[condition])
dnl
dnl this macro checks that gtk+-2.0 and gtk+-3.0 libraries are not mixed

AC_DEFUN([NA_CHECK_MODULE],[
	PKG_CHECK_MODULES([$1],[$2])
	#echo "cflags='${$1_CFLAGS}'"
	#echo "libs='${$1_LIBS}'"
	if ! test -z "${$1_LIBS}"; then
		if test "${have_gtk3}" = "yes"; then
			if test "$(echo ${$1_LIBS} | grep gtk-x11-2.0)" != ""; then
				AC_MSG_ERROR([$1: compiling with Gtk+-3 but adresses Gtk+-2 libraries])
			fi
		else
			if test "$(echo ${$1_LIBS} | grep gtk-x11-3.0)" != ""; then
				AC_MSG_ERROR([$1: compiling with Gtk+-2 but adresses Gtk+-3 libraries])
			fi
		fi
				
		NAUTILUS_ACTIONS_CFLAGS="${NAUTILUS_ACTIONS_CFLAGS} ${$1_CFLAGS}"
		NAUTILUS_ACTIONS_LIBS="${NAUTILUS_ACTIONS_LIBS} ${$1_LIBS}"
	else
		AC_MSG_ERROR([condition $2 not satisfied])
	fi
])
