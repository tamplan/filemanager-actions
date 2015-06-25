# Nautilus-Actions
# A Nautilus extension which offers configurable context menu actions.
#
# Copyright (C) 2005 The GNOME Foundation
# Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
# Copyright (C) 2009-2015 Pierre Wieser and others (see AUTHORS)
#
# Nautilus-Actions is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# Nautilus-Actions is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Nautilus-Actions; see the file COPYING. If not, see
# <http://www.gnu.org/licenses/>.
#
# Authors:
#   Frederic Ruaudel <grumz@grumz.net>
#   Rodrigo Moya <rodrigo@gnome-db.org>
#   Pierre Wieser <pwieser@trychlos.org>
#   ... and many others (see AUTHORS)

provider_id = na-desktop

pkglib_LTLIBRARIES = libna-io-desktop.la

provider_datadir = $(pkgdatadir)/$(provider_id)

AM_CPPFLAGS += \
	-I $(top_srcdir)									\
	-I $(top_srcdir)/src								\
	$(NAUTILUS_ACTIONS_CFLAGS)							\
	-DG_LOG_DOMAIN=\"${NA_LOGDOMAIN_IO_DESKTOP}\"		\
	-DPROVIDER_ID=\"$(provider_id)\"					\
	-DPROVIDER_DATADIR=\"$(provider_datadir)\"			\
	$(NULL)

libna_io_desktop_la_SOURCES = \
	nadp-desktop-file.c									\
	nadp-desktop-file.h									\
	nadp-desktop-provider.c								\
	nadp-desktop-provider.h								\
	nadp-formats.c										\
	nadp-formats.h										\
	nadp-keys.c											\
	nadp-keys.h											\
	nadp-module.c										\
	nadp-monitor.c										\
	nadp-monitor.h										\
	nadp-reader.c										\
	nadp-reader.h										\
	nadp-utils.c										\
	nadp-utils.h										\
	nadp-writer.c										\
	nadp-writer.h										\
	nadp-xdg-dirs.c										\
	nadp-xdg-dirs.h										\
	$(NULL)

libna_io_desktop_la_LIBADD = \
	$(top_builddir)/src/core/libna-core.la				\
	$(NAUTILUS_ACTIONS_LIBS)							\
	$(NULL)

libna_io_desktop_la_LDFLAGS = \
	-module 											\
	-no-undefined										\
	-avoid-version										\
	$(NULL)

images_files = \
	export-desktop.png									\
	$(NULL)

provider_data_DATA = \
	$(images_files)										\
	$(NULL)

EXTRA_DIST = \
	$(provider_data_DATA)								\
	$(NULL)
