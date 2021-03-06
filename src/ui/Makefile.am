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

bin_PROGRAMS = \
	fma-config-tool										\
	$(NULL)

ui_datadir = $(datadir)/fma-config-tool

egg_platform_defines = -DEGG_SM_CLIENT_BACKEND_XSMP

AM_CPPFLAGS += \
	-I $(top_srcdir)									\
	-I $(top_srcdir)/src								\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"			\
	-DG_LOG_DOMAIN=\"FMA\"								\
	-DPKGUIDIR=\"$(ui_datadir)\"						\
	$(egg_platform_defines)								\
	$(NAUTILUS_ACTIONS_CFLAGS)							\
	$(CODE_COVERAGE_CFLAGS)								\
	$(NULL)

BUILT_SOURCES = \
	$(NULL)

fma_config_tool_SOURCES = \
	base-assistant.c									\
	base-assistant.h									\
	base-builder.c										\
	base-builder.h										\
	base-dialog.c										\
	base-dialog.h										\
	base-gtk-utils.c									\
	base-gtk-utils.h									\
	base-keysyms.h										\
	base-window.c										\
	base-window.h										\
	egg-desktop-file.c									\
	egg-desktop-file.h									\
	egg-tree-multi-dnd.c								\
	egg-tree-multi-dnd.h								\
	main.c												\
	fma-add-capability-dialog.c							\
	fma-add-capability-dialog.h							\
	fma-add-scheme-dialog.c								\
	fma-add-scheme-dialog.h								\
	fma-application.c									\
	fma-application.h									\
	fma-assistant-export.c								\
	fma-assistant-export.h								\
	fma-assistant-import.c								\
	fma-assistant-import.h								\
	fma-clipboard.c										\
	fma-clipboard.h										\
	fma-export-ask.c									\
	fma-export-ask.h									\
	fma-iaction-tab.c									\
	fma-iaction-tab.h									\
	fma-ibasenames-tab.c								\
	fma-ibasenames-tab.h								\
	fma-icapabilities-tab.c								\
	fma-icapabilities-tab.h								\
	fma-icommand-tab.c									\
	fma-icommand-tab.h									\
	fma-icon-chooser.c									\
	fma-icon-chooser.h									\
	fma-ienvironment-tab.c								\
	fma-ienvironment-tab.h								\
	fma-iexecution-tab.c								\
	fma-iexecution-tab.h								\
	fma-ifolders-tab.c									\
	fma-ifolders-tab.h									\
	fma-imimetypes-tab.c								\
	fma-imimetypes-tab.h								\
	fma-iproperties-tab.c								\
	fma-iproperties-tab.h								\
	fma-ischemes-tab.c									\
	fma-ischemes-tab.h									\
	fma-main-tab.c										\
	fma-main-tab.h										\
	fma-main-window.c									\
	fma-main-window.h									\
	fma-main-window-def.h								\
	fma-match-list.c									\
	fma-match-list.h									\
	fma-menu.c											\
	fma-menu.h											\
	fma-menu-edit.c										\
	fma-menu-edit.h										\
	fma-menu-file.c										\
	fma-menu-file.h										\
	fma-menu-maintainer.c								\
	fma-menu-maintainer.h								\
	fma-menu-tools.c									\
	fma-menu-tools.h									\
	fma-menu-view.c										\
	fma-menu-view.h										\
	fma-preferences-editor.c							\
	fma-preferences-editor.h							\
	fma-providers-list.c								\
	fma-providers-list.h								\
	fma-schemes-list.c									\
	fma-schemes-list.h									\
	fma-sort-buttons.c									\
	fma-sort-buttons.h									\
	fma-status-bar.c									\
	fma-status-bar.h									\
	fma-tree-ieditable.c								\
	fma-tree-ieditable.h								\
	fma-tree-model.c									\
	fma-tree-model.h									\
	fma-tree-model-priv.h								\
	fma-tree-model-dnd.c								\
	fma-tree-view.c										\
	fma-tree-view.h										\
	$(BUILT_SOURCES)									\
	$(NULL)

fma_config_tool_LDADD = \
	$(top_builddir)/src/core/libfma-core.la				\
	$(NAUTILUS_ACTIONS_LIBS)							\
	$(NULL)

fma_config_tool_LDFLAGS = \
	$(CODE_COVERAGE_LDFLAGS)							\
	$(NULL)

actions_files = \
	fma-config-tool.actions								\
	$(NULL)

ui_files = \
	fma-add-capability.ui								\
	fma-add-scheme.ui									\
	fma-assistant-export.ui								\
	fma-assistant-import.ui								\
	fma-export-ask.ui									\
	fma-icon-chooser.ui									\
	fma-legend.ui										\
	fma-main-window.ui									\
	fma-preferences.ui									\
	fma-toolbar.ui										\
	$(NULL)

images_files = \
	gtk-convert.png										\
	gtk-index.png										\
	locked.png											\
	transparent.png										\
	$(NULL)

ui_data_DATA = \
	$(actions_files)									\
	$(ui_files)											\
	$(images_files)										\
	$(NULL)

@INTLTOOL_DESKTOP_RULE@

applications_in_files = fma-config-tool.desktop.in

applications_files = $(applications_in_files:.desktop.in=.desktop)

applicationsdir = $(datadir)/applications

applications_DATA = $(applications_files)

CLEANFILES = \
	$(applications_files)								\
	$(BUILT_SOURCES)									\
	$(NULL)

EXTRA_DIST = \
	$(applications_in_files)							\
	$(ui_data_DATA)										\
	$(NULL)

# Code coverage
@CODE_COVERAGE_RULES@
