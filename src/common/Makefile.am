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

noinst_LTLIBRARIES = libnact.la

AM_CPPFLAGS += \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-DG_LOG_DOMAIN=\"nact\"						\
	$(NAUTILUS_ACTIONS_CFLAGS)					\
	$(NULL)

libnact_la_SOURCES = \
	na-action.c									\
	na-action.h									\
	na-action-class.h							\
	na-action-profile.c							\
	na-action-profile.h							\
	na-action-profile-class.h					\
	na-gconf.c									\
	na-gconf.h									\
	na-gconf-keys.h								\
	na-iduplicable.c							\
	na-iduplicable.h							\
	na-iio-provider.c							\
	na-iio-provider.h							\
	na-ipivot-consumer.c						\
	na-ipivot-consumer.h						\
	na-object.c									\
	na-object.h									\
	na-pivot.c									\
	na-pivot.h									\
	na-utils.c									\
	na-utils.h									\
	$(NULL)

libnact_la_LIBADD = -lxml2 -luuid
