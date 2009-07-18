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
	nact-application.c									\
	nact-application.h									\
	nact-assistant.c									\
	nact-assistant.h									\
	nact-assist-export.c								\
	nact-assist-export.h								\
	nact-assist-import.c								\
	nact-assist-import.h								\
	nact-gconf-keys.h									\
	nact-gconf-reader.c									\
	nact-gconf-reader.h									\
	nact-gconf-writer.c									\
	nact-gconf-writer.h									\
	nact-iaction-tab.c									\
	nact-iaction-tab.h									\
	nact-iactions-list.c								\
	nact-iactions-list.h								\
	nact-iconditions.c									\
	nact-iconditions.h									\
	nact-iprefs.c										\
	nact-iprefs.h										\
	nact-iprofile-item.c								\
	nact-iprofile-item.h								\
	nact-iprofiles-list.c								\
	nact-iprofiles-list.h								\
	nact-main.c											\
	nact-main-window.c									\
	nact-main-window.h									\
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
