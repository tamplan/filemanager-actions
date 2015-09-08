# FileManager-Actions
# A file-manager extension which offers configurable context menu actions.
#
# Copyright (C) 2005 The GNOME Foundation
# Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
# Copyright (C) 2009-2015 Pierre Wieser and others (see AUTHORS)
#
# FileManager-Actions is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# FileManager-Actions is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with FileManager-Actions; see the file COPYING. If not, see
# <http://www.gnu.org/licenses/>.
#
# Authors:
#   Frederic Ruaudel <grumz@grumz.net>
#   Rodrigo Moya <rodrigo@gnome-db.org>
#   Pierre Wieser <pwieser@trychlos.org>
#   ... and many others (see AUTHORS)

provider_id = io-desktop

pkglib_LTLIBRARIES = libfma-io-desktop.la

provider_datadir = $(pkgdatadir)/$(provider_id)

AM_CPPFLAGS += \
	-I $(top_srcdir)									\
	-I $(top_srcdir)/src								\
	$(NAUTILUS_ACTIONS_CFLAGS)							\
	-DG_LOG_DOMAIN=\"${NA_LOGDOMAIN_IO_DESKTOP}\"		\
	-DPROVIDER_ID=\"$(provider_id)\"					\
	-DPROVIDER_DATADIR=\"$(provider_datadir)\"			\
	$(NULL)

libfma_io_desktop_la_SOURCES = \
	fma-desktop-file.c									\
	fma-desktop-file.h									\
	fma-desktop-provider.c								\
	fma-desktop-provider.h								\
	fma-desktop-formats.c								\
	fma-desktop-formats.h								\
	fma-desktop-keys.c									\
	fma-desktop-keys.h									\
	fma-desktop-module.c								\
	fma-desktop-monitor.c								\
	fma-desktop-monitor.h								\
	fma-desktop-reader.c								\
	fma-desktop-reader.h								\
	fma-desktop-utils.c									\
	fma-desktop-utils.h									\
	fma-desktop-writer.c								\
	fma-desktop-writer.h								\
	fma-desktop-xdg-dirs.c								\
	fma-desktop-xdg-dirs.h								\
	$(NULL)

libfma_io_desktop_la_LIBADD = \
	$(top_builddir)/src/core/libfma-core.la				\
	$(NAUTILUS_ACTIONS_LIBS)							\
	$(NULL)

libfma_io_desktop_la_LDFLAGS = \
	-module 											\
	-no-undefined										\
	-avoid-version										\
	$(NULL)

images_files = \
	fma-desktop-export.png								\
	$(NULL)

provider_data_DATA = \
	$(images_files)										\
	$(NULL)

EXTRA_DIST = \
	$(provider_data_DATA)								\
	$(NULL)
