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

bin_PROGRAMS = \
	nautilus-actions-config								\
	$(NULL)

gladedir = $(datadir)/nautilus-actions

AM_CPPFLAGS += \
	-I $(top_srcdir)/src								\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"			\
	-DGLADEDIR=\"$(gladedir)\"							\
	$(NAUTILUS_ACTIONS_CFLAGS)							\
	$(NULL)

nautilus_actions_config_SOURCES = \
	base-application.c									\
	base-application.h									\
	base-window.c										\
	base-window.h										\
	nact-action-conditions-editor.c						\
	nact-action-conditions-editor.h						\
	nact-action-profiles-editor.c						\
	nact-action-profiles-editor.h						\
	nact-application.c									\
	nact-application.h									\
	nact-iactions-list.c								\
	nact-iactions-list.h								\
	nact-imenu-item.c									\
	nact-imenu-item.h									\
	nact-iprofile-conditions.c							\
	nact-iprofile-conditions.h							\
	nact-main.c											\
	nact-main-window.c									\
	nact-main-window.h									\
	nact-profile-conditions-editor.c					\
	nact-profile-conditions-editor.h					\
	nact-prefs.c										\
	nact-prefs.h										\
	nact-import-export.c								\
	nact-import-export.h								\
	nact-window.c										\
	nact-window.h										\
	$(NULL)

nautilus_actions_config_LDADD = \
	$(top_builddir)/src/common/libnact.la				\
	$(NAUTILUS_ACTIONS_LIBS)							\
	$(NULL)

glade_DATA = nautilus-actions-config.ui

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
	$(glade_DATA)										\
	$(NULL)
