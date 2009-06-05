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
	nact-action.c								\
	nact-action.h								\
	nact-action-profile.c						\
	nact-action-profile.h						\
	nact-gconf.c								\
	nact-gconf.h								\
	nact-gconf-keys.h							\
	nact-iio-provider.c							\
	nact-iio-provider.h							\
	nact-object.c								\
	nact-object.h								\
	nact-pivot.c								\
	nact-pivot.h								\
	nautilus-actions-config.c					\
	nautilus-actions-config.h					\
	nautilus-actions-config-gconf.c				\
	nautilus-actions-config-gconf.h				\
	nautilus-actions-config-gconf-private.h		\
	nautilus-actions-config-gconf-reader.c		\
	nautilus-actions-config-gconf-reader.h		\
	nautilus-actions-config-gconf-writer.c		\
	nautilus-actions-config-gconf-writer.h		\
	nautilus-actions-config-schema-writer.c		\
	nautilus-actions-config-schema-writer.h		\
	nautilus-actions-config-schema-reader.c		\
	nautilus-actions-config-schema-reader.h		\
	uti-lists.c									\
	uti-lists.h									\
	$(NULL)

libnact_la_LIBADD = -lxml2 -luuid
