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

pkglib_LTLIBRARIES = libna-core.la

AM_CPPFLAGS += \
	-I $(top_srcdir)									\
	-DPKGLIBDIR=\""$(pkglibdir)"\"						\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"			\
	-DG_LOG_DOMAIN=\"${NA_LOGDOMAIN_CORE}\"				\
	$(NAUTILUS_ACTIONS_CFLAGS)							\
	$(NULL)

libna_core_la_SOURCES = \
	na-core-utils.c										\
	na-data-element.c									\
	na-data-element.h									\
	na-data-factory.c									\
	na-data-factory.h									\
	na-dbus-tracker.h									\
	na-gconf-monitor.c									\
	na-gconf-utils.c									\
	na-iduplicable.c									\
	na-idata-factory.c									\
	na-iexporter.c										\
	na-iio-factory.c									\
	na-iio-provider.c									\
	na-io-factory.c										\
	na-io-factory.h										\
	na-object.c											\
	na-object-id.c										\
	na-object-id-enum.c									\
	na-object-item.c									\
	na-object-item-enum.c								\
	na-object-action.c									\
	na-object-action-enum.c								\
	na-object-profile.c									\
	na-object-profile-enum.c							\
	na-object-menu.c									\
	na-object-menu-enum.c								\
	\
	na-iabout.c											\
	na-iabout.h											\
	na-io-provider.h									\
	na-ipivot-consumer.c								\
	na-ipivot-consumer.h								\
	na-iprefs.c											\
	na-iprefs.h											\
	na-module.c											\
	na-module.h											\
	na-pivot.c											\
	na-pivot.h											\
	na-utils.c											\
	na-utils.h											\
	na-xml-names.h										\
	na-xml-writer.c										\
	na-xml-writer.h										\
	$(NULL)

libna_core_la_LIBADD = \
	$(NULL)

VERSION_INFO = `echo @PACKAGE_VERSION@ | sed 's/\./:/g'`
libna_core_la_LDFLAGS = \
	-no-undefined										\
	-version-number $(VERSION_INFO)						\
	$(NULL)
