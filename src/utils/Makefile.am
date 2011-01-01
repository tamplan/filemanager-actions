# Nautilus Actions
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

bin_PROGRAMS = \
	nautilus-actions-new										\
	nautilus-actions-run										\
	nautilus-actions-schemas									\
	$(NULL)

AM_CPPFLAGS += \
	-I $(top_srcdir)											\
	-I $(top_srcdir)/src										\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"					\
	-DG_LOG_DOMAIN=\"${NA_LOGDOMAIN_UTILS}\"					\
	$(NAUTILUS_ACTIONS_CFLAGS)									\
	$(NULL)

nautilus_actions_new_SOURCES = \
	nautilus-actions-new.c										\
	console-utils.c												\
	console-utils.h												\
	$(NULL)

nautilus_actions_new_LDADD = \
	$(top_builddir)/src/core/libna-core.la						\
	$(NAUTILUS_ACTIONS_LIBS)									\
	$(NULL)

BUILT_SOURCES = \
	nautilus-actions-run-bindings.h								\
	$(NULL)

nautilus-actions-run-bindings.h: $(top_srcdir)/src/plugin-tracker/na-tracker-dbus.xml
	dbus-binding-tool --mode=glib-client $< > $@

nautilus_actions_run_SOURCES = \
	nautilus-actions-run.c										\
	console-utils.c												\
	console-utils.h												\
	$(NULL)

nautilus_actions_run_LDADD = \
	$(top_builddir)/src/core/libna-core.la						\
	$(NAUTILUS_ACTIONS_LIBS)									\
	$(NULL)

nautilus_actions_schemas_SOURCES = \
	nautilus-actions-schemas.c									\
	console-utils.c												\
	console-utils.h												\
	$(NULL)

nautilus_actions_schemas_LDADD = \
	$(top_builddir)/src/core/libna-core.la						\
	$(NAUTILUS_ACTIONS_LIBS)									\
	$(NULL)

CLEANFILES = \
	$(BUILT_SOURCES)											\
	$(NULL)
