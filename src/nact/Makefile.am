# Nautilus-Actions
# A Nautilus extension which offers configurable context menu actions.
#
# Copyright (C) 2005 The GNOME Foundation
# Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
# Copyright (C) 2009-2013 Pierre Wieser and others (see AUTHORS)
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

bin_PROGRAMS = \
	nautilus-actions-config-tool						\
	$(NULL)

ui_datadir = $(pkgdatadir)/ui

egg_platform_defines = -DEGG_SM_CLIENT_BACKEND_XSMP

AM_CPPFLAGS += \
	-I $(top_srcdir)									\
	-I $(top_srcdir)/src								\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"			\
	-DG_LOG_DOMAIN=\"${NA_LOGDOMAIN_NACT}\"				\
	-DPKGUIDIR=\"$(ui_datadir)\"						\
	$(egg_platform_defines)								\
	$(NAUTILUS_ACTIONS_CFLAGS)							\
	$(NULL)

BUILT_SOURCES = \
	base-marshal.c										\
	base-marshal.h										\
	nact-marshal.c										\
	nact-marshal.h										\
	$(NULL)

nautilus_actions_config_tool_SOURCES = \
	base-application.c									\
	base-application.h									\
	base-assistant.c									\
	base-assistant.h									\
	base-builder.c										\
	base-builder.h										\
	base-dialog.c										\
	base-dialog.h										\
	base-gtk-utils.c									\
	base-gtk-utils.h									\
	base-isession.c										\
	base-isession.h										\
	base-iunique.c										\
	base-iunique.h										\
	base-keysyms.h										\
	base-window.c										\
	base-window.h										\
	egg-desktop-file.c									\
	egg-desktop-file.h									\
	egg-sm-client.c										\
	egg-sm-client.h										\
	egg-sm-client-private.h								\
	egg-sm-client-xsmp.c								\
	egg-tree-multi-dnd.c								\
	egg-tree-multi-dnd.h								\
	main.c												\
	nact-add-capability-dialog.c						\
	nact-add-capability-dialog.h						\
	nact-add-scheme-dialog.c							\
	nact-add-scheme-dialog.h							\
	nact-application.c									\
	nact-application.h									\
	nact-assistant-export.c								\
	nact-assistant-export.h								\
	nact-assistant-import.c								\
	nact-assistant-import.h								\
	nact-clipboard.c									\
	nact-clipboard.h									\
	nact-confirm-logout.c								\
	nact-confirm-logout.h								\
	nact-export-ask.c									\
	nact-export-ask.h									\
	nact-iaction-tab.c									\
	nact-iaction-tab.h									\
	nact-ibasenames-tab.c								\
	nact-ibasenames-tab.h								\
	nact-icapabilities-tab.c							\
	nact-icapabilities-tab.h							\
	nact-icon-chooser.c									\
	nact-icon-chooser.h									\
	nact-ienvironment-tab.c								\
	nact-ienvironment-tab.h								\
	nact-iexecution-tab.c								\
	nact-iexecution-tab.h								\
	nact-ifolders-tab.c									\
	nact-ifolders-tab.h									\
	nact-icommand-tab.c									\
	nact-icommand-tab.h									\
	nact-imimetypes-tab.c								\
	nact-imimetypes-tab.h								\
	nact-iproperties-tab.c								\
	nact-iproperties-tab.h								\
	nact-ischemes-tab.c									\
	nact-ischemes-tab.h									\
	nact-main-statusbar.c								\
	nact-main-statusbar.h								\
	nact-main-tab.c										\
	nact-main-tab.h										\
	nact-main-toolbar.c									\
	nact-main-toolbar.h									\
	nact-main-window.c									\
	nact-main-window.h									\
	nact-match-list.c									\
	nact-match-list.h									\
	nact-menubar.c										\
	nact-menubar.h										\
	nact-menubar-priv.h									\
	nact-menubar-edit.c									\
	nact-menubar-file.c									\
	nact-menubar-help.c									\
	nact-menubar-maintainer.c							\
	nact-menubar-tools.c								\
	nact-menubar-view.c									\
	nact-preferences-editor.c							\
	nact-preferences-editor.h							\
	nact-providers-list.c								\
	nact-providers-list.h								\
	nact-schemes-list.c									\
	nact-schemes-list.h									\
	nact-sort-buttons.c									\
	nact-sort-buttons.h									\
	nact-tree-ieditable.c								\
	nact-tree-ieditable.h								\
	nact-tree-model.c									\
	nact-tree-model.h									\
	nact-tree-model-priv.h								\
	nact-tree-model-dnd.c								\
	nact-tree-view.c									\
	nact-tree-view.h									\
	$(BUILT_SOURCES)									\
	$(NULL)

base-marshal.h: base-marshal.def $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=base_cclosure_marshal > $@

base-marshal.c: base-marshal.def $(GLIB_GENMARSHAL)
	echo "#include \"base-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=base_cclosure_marshal >> $@

nact-marshal.h: nact-marshal.def $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=nact_cclosure_marshal > $@

nact-marshal.c: nact-marshal.def $(GLIB_GENMARSHAL)
	echo "#include \"nact-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=nact_cclosure_marshal >> $@

nautilus_actions_config_tool_LDADD = \
	$(top_builddir)/src/core/libna-core.la				\
	$(NAUTILUS_ACTIONS_LIBS)							\
	$(NULL)

actions_files = \
	nautilus-actions-config-tool.actions				\
	nautilus-actions-maintainer.actions					\
	$(NULL)

ui_files = \
	nact-add-capability.ui								\
	nact-add-scheme.ui									\
	nact-assistant-export.ui							\
	nact-assistant-import.ui							\
	nact-icon-chooser.ui								\
	nact-preferences.ui									\
	nautilus-actions-config-tool.ui						\
	$(NULL)

images_files = \
	locked.png											\
	transparent.png										\
	$(NULL)

ui_data_DATA = \
	$(actions_files)									\
	$(ui_files)											\
	$(images_files)										\
	$(NULL)

@INTLTOOL_DESKTOP_RULE@

applications_in_files = nact.desktop.in

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
	base-marshal.def									\
	nact-marshal.def									\
	$(NULL)
