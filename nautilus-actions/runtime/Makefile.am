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

pkglib_LTLIBRARIES = libna-runtime.la

AM_CPPFLAGS += \
	-I $(top_srcdir)							\
	-DPKGLIBDIR=\""$(pkglibdir)"\"				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-DG_LOG_DOMAIN=\"${NA_LOGDOMAIN_RUNTIME}\"	\
	$(NAUTILUS_ACTIONS_CFLAGS)					\
	$(NULL)

libna_runtime_la_SOURCES = \
	na-gconf-keys.h								\
	na-gconf-monitor.c							\
	na-gconf-monitor.h							\
	na-gconf-provider.c							\
	na-gconf-provider.h							\
	na-gconf-provider-keys.h					\
	na-gconf-utils.c							\
	na-gconf-utils.h							\
	na-gnome-vfs-uri.c							\
	na-gnome-vfs-uri.h							\
	na-iabout.c									\
	na-iabout.h									\
	na-iduplicable.c							\
	na-iduplicable.h							\
	na-io-provider.c							\
	na-io-provider.h							\
	na-ipivot-consumer.c						\
	na-ipivot-consumer.h						\
	na-iprefs.c									\
	na-iprefs.h									\
	na-module.c									\
	na-module.h									\
	na-object-api.h								\
	na-object.c									\
	na-object-class.h							\
	na-object-fn.h								\
	na-object-priv.h							\
	na-object-id.c								\
	na-object-id-class.h						\
	na-object-id-fn.h							\
	na-object-id-priv.h							\
	na-object-item.c							\
	na-object-item-class.h						\
	na-object-item-fn.h							\
	na-object-item-priv.h						\
	na-object-action.c							\
	na-object-action-class.h					\
	na-object-action-fn.h						\
	na-object-action-priv.h						\
	na-object-menu.c							\
	na-object-menu-class.h						\
	na-object-menu-fn.h							\
	na-object-menu-priv.h						\
	na-object-profile.c							\
	na-object-profile-class.h					\
	na-object-profile-fn.h						\
	na-object-profile-priv.h					\
	na-pivot.c									\
	na-pivot.h									\
	na-utils.c									\
	na-utils.h									\
	$(NULL)

libna_runtime_la_LIBADD = \
	$(NULL)

VERSION_INFO = 1:0:0
libna_runtime_la_LDFLAGS = \
	-module										\
	-no-undefined								\
	-version-number $(VERSION_INFO)				\
	$(NULL)
