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

AM_CPPFLAGS += \
	-I $(top_srcdir)											\
	-I $(top_srcdir)/src										\
	-DG_LOG_DOMAIN=\"${NA_LOGDOMAIN_PLUGIN_TRACKER}\"			\
	$(NAUTILUS_ACTIONS_CFLAGS)									\
	$(NULL)

BUILT_SOURCES =

BUILT_SOURCES += \
	fma-tracker-gdbus.c											\
	fma-tracker-gdbus.h											\
	$(NULL)

fma-tracker-gdbus.c fma-tracker-gdbus.h: fma-tracker-gdbus.xml
	gdbus-codegen \
		--interface-prefix org.filemanager_actions.DBus.Tracker.	\
		--generate-c-code fma-tracker-gdbus							\
		--c-namespace FMA_Tracker_GDBus								\
		--c-generate-object-manager									\
		--generate-docbook fma-tracker-gdbus-docs					\
		$<

DISTCLEANFILES = \
	fma-tracker-gdbus-docs-org.filemanager_actions.DBus.Tracker.Properties1.xml

nodist_libfma_sources = \
	$(BUILT_SOURCES)											\
	$(NULL)

libfma_sources = \
	fma-tracker-module.c										\
	fma-tracker-plugin.h										\
	fma-tracker-plugin.c										\
	$(NULL)

libfma_ldadd = \
	$(top_builddir)/src/core/libfma-core.la						\
	$(NAUTILUS_ACTIONS_LIBS)									\
	$(NULL)

libfma_ldflags = \
	-module 													\
	-no-undefined												\
	-avoid-version												\
	$(NULL)

if HAVE_NAUTILUS
nautilus_extensiondir = $(NAUTILUS_EXTENSIONS_DIR)
nautilus_extension_LTLIBRARIES = libfma-nautilus-tracker.la
nodist_libfma_nautilus_tracker_la_SOURCES = $(nodist_libfma_sources)
libfma_nautilus_tracker_la_SOURCES = $(libfma_sources)
libfma_nautilus_tracker_la_LIBADD = $(libfma_ldadd)
libfma_nautilus_tracker_la_LDFLAGS = $(libfma_ldflags)
libfma_nautilus_tracker_la_CPPFLAGS = $(AM_CPPFLAGS)			\
	-DFMA_TARGET_ID=$(NAUTILUS_ID)								\
	-DFMA_TARGET_LABEL=\"$(NAUTILUS_LABEL)\"					\
	$(NULL)
endif

if HAVE_NEMO
nemo_extensiondir = $(NEMO_EXTENSIONS_DIR)
nemo_extension_LTLIBRARIES = libfma-nemo-tracker.la
nodist_libfma_nemo_tracker_la_SOURCES = $(nodist_libfma_sources)
libfma_nemo_tracker_la_SOURCES = $(libfma_sources)
libfma_nemo_tracker_la_LIBADD = $(libfma_ldadd)
libfma_nemo_tracker_la_LDFLAGS = $(libfma_ldflags)
libfma_nemo_tracker_la_CPPFLAGS = $(AM_CPPFLAGS)				\
	-DFMA_TARGET_ID=$(NEMO_ID)									\
	-DFMA_TARGET_LABEL=\"$(NEMO_LABEL)\"						\
	$(NULL)
endif

EXTRA_DIST = \
	fma-tracker-gdbus.xml										\
	$(NULL)

CLEANFILES = \
	$(BUILT_SOURCES)											\
	$(NULL)
