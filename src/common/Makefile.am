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
	-DG_LOG_DOMAIN=\"${NA_LOGDOMAIN_COMMON}\"	\
	$(NAUTILUS_ACTIONS_CFLAGS)					\
	$(NULL)

libnact_la_SOURCES = \
	na-about.c									\
	na-about.h									\
	na-gconf-keys-base.h						\
	na-gconf-keys-schemas.h						\
	na-gconf-monitor.c							\
	na-gconf-monitor.h							\
	na-gconf-provider.c							\
	na-gconf-provider.h							\
	na-gconf-provider-keys.h					\
	na-gconf-utils.c							\
	na-gconf-utils.h							\
	na-gnome-vfs-uri.c							\
	na-gnome-vfs-uri.h							\
	na-iduplicable.c							\
	na-iduplicable.h							\
	na-iio-provider.c							\
	na-iio-provider.h							\
	na-ipivot-consumer.c						\
	na-ipivot-consumer.h						\
	na-iprefs.c									\
	na-iprefs.h									\
	na-object.c									\
	na-object-api.h								\
	na-object-class.h							\
	na-object-fn.h								\
	na-object-id.c								\
	na-object-id-class.h						\
	na-object-id-fn.h							\
	na-object-item.c							\
	na-object-item-class.h						\
	na-object-item-fn.h							\
	na-obj-action.c								\
	na-obj-action.h								\
	na-obj-action-class.h						\
	na-obj-menu.c								\
	na-obj-menu.h								\
	na-obj-profile.c							\
	na-obj-profile.h							\
	na-obj-profile-class.h						\
	na-pivot.c									\
	na-pivot.h									\
	na-utils.c									\
	na-utils.h									\
	na-xml-names.h								\
	na-xml-writer.c								\
	na-xml-writer.h								\
	$(NULL)

libnact_la_LIBADD = \
	-lxml2										\
	-luuid										\
	$(NULL)
