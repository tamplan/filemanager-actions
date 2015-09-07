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

pkglib_LTLIBRARIES = libna-core.la

exportformat_datadir = $(pkgdatadir)/export-format

importmode_datadir = $(pkgdatadir)/import-mode

icon_fname = $(datadir)/icons/hicolor/48x48/apps/file-manager-actions.png

ui_datadir = $(pkgdatadir)/ui

AM_CPPFLAGS += \
	-I $(top_srcdir)									\
	-I $(top_srcdir)/src								\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"			\
	-DPKGLIBDIR=\""$(pkglibdir)"\"						\
	-DPKGLIBEXECDIR=\""$(pkglibexecdir)"\"				\
	-DSYSCONFDIR=\"$(sysconfdir)\"						\
	-DG_LOG_DOMAIN=\"${NA_LOGDOMAIN_CORE}\"				\
	-DICON_FNAME=\"$(icon_fname)\"						\
	-DPKGEXPORTFORMATDIR=\"$(exportformat_datadir)\"	\
	-DPKGIMPORTMODEDIR=\"$(importmode_datadir)\"		\
	-DPKGUIDIR=\"$(ui_datadir)\"						\
	$(NAUTILUS_ACTIONS_CFLAGS)							\
	$(NULL)

libna_core_la_SOURCES = \
	fma-about.c											\
	fma-about.h											\
	fma-boxed.c											\
	fma-core-utils.c										\
	fma-data-boxed.c										\
	fma-data-def.c										\
	fma-data-types.c										\
	fma-desktop-environment.c							\
	fma-desktop-environment.h							\
	fma-exporter.c										\
	fma-exporter.h										\
	fma-export-format.c									\
	fma-export-format.h									\
	na-factory-object.c									\
	na-factory-object.h									\
	na-factory-provider.c								\
	na-factory-provider.h								\
	na-gconf-migration.c								\
	na-gconf-migration.h								\
	fma-gconf-monitor.c									\
	fma-gconf-utils.c									\
	na-gnome-vfs-uri.c									\
	na-gnome-vfs-uri.h									\
	na-gtk-utils.c										\
	na-gtk-utils.h										\
	fma-icontext.c										\
	fma-icontext-factory.c								\
	fma-iduplicable.c									\
	fma-iexporter.c										\
	fma-ifactory-object.c								\
	fma-ifactory-provider.c								\
	fma-iimporter.c										\
	fma-iio-provider.c									\
	na-import-mode.c									\
	na-import-mode.h									\
	na-importer.c										\
	na-importer.h										\
	na-importer-ask.c									\
	na-importer-ask.h									\
	na-io-provider.c									\
	na-io-provider.h									\
	na-ioption.c										\
	na-ioption.h										\
	na-ioptions-list.c									\
	na-ioptions-list.h									\
	na-iprefs.c											\
	na-iprefs.h											\
	na-module.c											\
	na-module.h											\
	fma-object.c											\
	fma-object-id.c										\
	fma-object-id-factory.c								\
	fma-object-item.c									\
	fma-object-item-factory.c							\
	fma-object-action.c									\
	fma-object-action-factory.c							\
	fma-object-profile.c									\
	fma-object-profile-factory.c							\
	fma-object-menu.c									\
	fma-object-menu-factory.c							\
	na-pivot.c											\
	na-pivot.h											\
	na-selected-info.c									\
	na-selected-info.h									\
	na-settings.c										\
	na-settings.h										\
	fma-timeout.c										\
	na-tokens.c											\
	na-tokens.h											\
	na-updater.c										\
	na-updater.h										\
	$(BUILT_SOURCES)									\
	$(NULL)

libna_core_la_LIBADD = \
	$(NAUTILUS_ACTIONS_LIBS)							\
	$(NULL)

libna_core_la_LDFLAGS = \
	-no-undefined										\
	-avoid-version										\
	-fPIC												\
	$(NULL)

ui_data_DATA = \
	na-importer-ask.ui									\
	$(NULL)

exportformat_data_DATA = \
	export-format-ask.png								\
	$(NULL)

importmode_data_DATA = \
	import-mode-ask.png									\
	import-mode-no-import.png							\
	import-mode-override.png							\
	import-mode-renumber.png							\
	$(NULL)

CLEANFILES = \
	$(NULL)

EXTRA_DIST = \
	$(ui_data_DATA)										\
	$(exportformat_data_DATA)							\
	$(importmode_data_DATA)								\
	$(NULL)
