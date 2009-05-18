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

bin_PROGRAMS = \
	nautilus-actions-config

AM_CPPFLAGS += \
	-I $(top_srcdir)									\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"			\
	-DGLADEDIR=\""$(datadir)/nautilus-actions/nact"\"	\
	$(NAUTILUS_ACTIONS_CFLAGS)

nautilus_actions_config_SOURCES = \
	nact.c												\
	nact.h												\
	nact-utils.c										\
	nact-utils.h										\
	nact-prefs.c										\
	nact-prefs.h										\
	nact-import-export.c								\
	nact-import-export.h								\
	nact-profile-editor.c								\
	nact-profile-editor.h								\
	nact-editor.c										\
	nact-editor.h

nautilus_actions_config_LDADD = \
	$(top_builddir)/libnautilus-actions/libnautilus-actions.la	\
	$(NAUTILUS_ACTIONS_LIBS)

gladedir = $(datadir)/nautilus-actions/nact

glade_DATA = nautilus-actions-config.glade

@INTLTOOL_DESKTOP_RULE@

Applications_in_files = nact.desktop.in

Applications_files = $(Applications_in_files:.desktop.in=.desktop)

Applicationsdir = $(datadir)/applications

schemedir = $(datadir)/applications

scheme_DATA = $(Applications_files)

DISTCLEANFILES = \
	$(Applications_files)

EXTRA_DIST = \
	$(Applications_in_files)							\
	$(glade_DATA)
