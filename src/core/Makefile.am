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

pkglib_LTLIBRARIES = libfma-core.la

exportformat_datadir = $(pkgdatadir)/export-format

importmode_datadir = $(pkgdatadir)/import-mode

importerask_datadir = $(pkgdatadir)/importer-ask

icon_fname = $(datadir)/icons/hicolor/48x48/apps/filemanager-actions.png

AM_CPPFLAGS += \
	-I $(top_srcdir)									\
	-I $(top_srcdir)/src								\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"			\
	-DPKGLIBDIR=\""$(pkglibdir)"\"						\
	-DPKGLIBEXECDIR=\""$(pkglibexecdir)"\"				\
	-DG_LOG_DOMAIN=\"FMA\"								\
	-DICON_FNAME=\"$(icon_fname)\"						\
	-DPKGEXPORTFORMATDIR=\"$(exportformat_datadir)\"	\
	-DPKGIMPORTMODEDIR=\"$(importmode_datadir)\"		\
	-DPKGIMPORTERASKDIR=\"$(importerask_datadir)\"		\
	$(NAUTILUS_ACTIONS_CFLAGS)							\
	$(CODE_COVERAGE_CFLAGS)								\
	$(NULL)

libfma_core_la_SOURCES = \
	fma-about.c											\
	fma-about.h											\
	fma-boxed.c											\
	fma-core-utils.c									\
	fma-data-boxed.c									\
	fma-data-def.c										\
	fma-data-types.c									\
	fma-desktop-environment.c							\
	fma-desktop-environment.h							\
	fma-exporter.c										\
	fma-exporter.h										\
	fma-export-format.c									\
	fma-export-format.h									\
	fma-factory-object.c								\
	fma-factory-object.h								\
	fma-factory-provider.c								\
	fma-factory-provider.h								\
	fma-gconf-migration.c								\
	fma-gconf-migration.h								\
	fma-gconf-monitor.c									\
	fma-gconf-utils.c									\
	fma-gnome-vfs-uri.c									\
	fma-gnome-vfs-uri.h									\
	fma-gtk-utils.c										\
	fma-gtk-utils.h										\
	fma-icontext.c										\
	fma-icontext-factory.c								\
	fma-iduplicable.c									\
	fma-iexporter.c										\
	fma-ifactory-object.c								\
	fma-ifactory-provider.c								\
	fma-iimporter.c										\
	fma-iio-provider.c									\
	fma-import-mode.c									\
	fma-import-mode.h									\
	fma-importer.c										\
	fma-importer.h										\
	fma-importer-ask.c									\
	fma-importer-ask.h									\
	fma-io-provider.c									\
	fma-io-provider.h									\
	fma-ioption.c										\
	fma-ioption.h										\
	fma-ioptions-list.c									\
	fma-ioptions-list.h									\
	fma-iprefs.c										\
	fma-iprefs.h										\
	fma-module.c										\
	fma-module.h										\
	fma-object.c										\
	fma-object-id.c										\
	fma-object-id-factory.c								\
	fma-object-item.c									\
	fma-object-item-factory.c							\
	fma-object-action.c									\
	fma-object-action-factory.c							\
	fma-object-profile.c								\
	fma-object-profile-factory.c						\
	fma-object-menu.c									\
	fma-object-menu-factory.c							\
	fma-pivot.c											\
	fma-pivot.h											\
	fma-selected-info.c									\
	fma-selected-info.h									\
	fma-settings.c										\
	fma-settings.h										\
	fma-timeout.c										\
	fma-tokens.c										\
	fma-tokens.h										\
	fma-updater.c										\
	fma-updater.h										\
	$(BUILT_SOURCES)									\
	$(NULL)

libfma_core_la_LIBADD = \
	$(NAUTILUS_ACTIONS_LIBS)							\
	$(NULL)

libfma_core_la_LDFLAGS = \
	-no-undefined										\
	-avoid-version										\
	-fPIC												\
	$(CODE_COVERAGE_LDFLAGS)							\
	$(NULL)

importerask_data_DATA = \
	fma-importer-ask.ui									\
	$(NULL)

exportformat_data_DATA = \
	fma-export-format-ask.png							\
	$(NULL)

importmode_data_DATA = \
	fma-import-mode-ask.png								\
	fma-import-mode-no-import.png						\
	fma-import-mode-override.png						\
	fma-import-mode-renumber.png						\
	$(NULL)

CLEANFILES = \
	$(NULL)

EXTRA_DIST = \
	$(importerask_data_DATA)							\
	$(exportformat_data_DATA)							\
	$(importmode_data_DATA)								\
	$(NULL)

# Code coverage
@CODE_COVERAGE_RULES@
