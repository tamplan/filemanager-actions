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
#
# from Migrating your documentation to gnome-doc-utils
# http://live.gnome.org/GnomeDocUtilsMigrationHowTo
#
# The gnome-doc-utils packages produces the FileManager-Actions
# Configuration Tool Users's Manual as DocBook XML documents, suitable
# for Yelp, the Gnome help system.
#
# At install time, it installs these .xml help files
# in <datadir>/gnome/help/fma-config-tool/<lang>.
# Missing tranlated images are replaced with symlinks to
# ../../C/figures/<image.png>.
#
# --enable-html-manuals (resp. --enable-pdf-manuals) are configure
# options which produce HTML (resp. PDF) formats. These options are
# always enabled when making distcheck, so that all output formats for
# all known locales are always included in the distributed tarball.
#
# Do not confuse gnome-doc-utils which targets the help documentation
# of the Gnome (i.e. gui) applications, and that we are using here to
# generate html and pdf NACT user's manuals, with gtk-doc which rather
# targets the developer documentation (see docs/reference).
#
# Do not use the $(NULL) syntax here as this may prevent Damned-Lies
# to correctly generate POT files (cf. mail of Claude Paroz 2010-08-29
# http://mail.gnome.org/archives/gnome-i18n/2010-August/msg00229.html).

#include $(srcdir)/gnome-doc-utils-na.make
include $(top_srcdir)/gnome-doc-utils.make
dist-hook: doc-dist-hook

DOC_MODULE = fma-config-tool

#DOC_ENTITIES = $(patsubst $(srcdir)/C/%,%,$(shell \ls -1 $(srcdir)/C/*.xml | $(GREP) -v $(DOC_MODULE)))
DOC_ENTITIES = \
	appendix-gnu-fdl-1.3.xml						\
	articleinfo.xml									\
	fma-bugs.xml									\
	fma-conditions.xml								\
	fma-config-tool.xml								\
	fma-edit.xml									\
	fma-execution.xml								\
	fma-export.xml									\
	fma-import.xml									\
	fma-menus.xml									\
	fma-misc.xml									\
	fma-prefs.xml									\
	fma-profiles.xml								\
	fma-properties.xml								\
	fma-save.xml									\
	fma-sorting.xml									\
	fma-starting.xml								\
	fma-where.xml									\
	legalnotice.xml

#DOC_INCLUDES =

#DOC_FIGURES = $(patsubst $(srcdir)/C/%,%,$(shell \ls -1 $(srcdir)/C/figures/*.png))
DOC_FIGURES = \
	figures/add.png									\
	figures/fma-action-tab.png						\
	figures/fma-basenames-tab.png					\
	figures/fma-capabilities-tab.png				\
	figures/fma-command-tab.png						\
	figures/fma-desktop-environment-frame.png		\
	figures/fma-environment-tab.png					\
	figures/fma-exec-mode-frame.png					\
	figures/fma-execute-as-frame.png				\
	figures/fma-execution-environment-frame.png		\
	figures/fma-execution-tab.png					\
	figures/fma-export-done.png						\
	figures/fma-export-format.png					\
	figures/fma-export-home.png						\
	figures/fma-export-items.png					\
	figures/fma-export-summary.png					\
	figures/fma-export-target.png					\
	figures/fma-folders-tab.png						\
	figures/fma-import-done.png						\
	figures/fma-import-files.png					\
	figures/fma-import-home.png						\
	figures/fma-import-mode.png						\
	figures/fma-import-resultat.png					\
	figures/fma-import-summary.png					\
	figures/fma-legend.png							\
	figures/fma-main-empty.png						\
	figures/fma-mimetypes-tab.png					\
	figures/fma-new-capability.png					\
	figures/fma-new-scheme.png						\
	figures/fma-preferences-desktop.png				\
	figures/fma-preferences-execution.png			\
	figures/fma-preferences-export.png				\
	figures/fma-preferences-import.png				\
	figures/fma-preferences-io-providers.png		\
	figures/fma-preferences-runtime.png				\
	figures/fma-preferences-schemes.png				\
	figures/fma-preferences-ui.png					\
	figures/fma-properties-tab.png					\
	figures/fma-schemes-tab.png						\
	figures/fma-selection-count-frame.png			\
	figures/fma-startup-mode-frame.png				\
	figures/fma-target-location.png					\
	figures/fma-working-directory-frame.png			\
	figures/read-only.png							\
	figures/remove.png								\
	figures/sort-asc.png							\
	figures/sort-buttons-frame.png					\
	figures/sort-desc.png							\
	figures/sort-manual.png							\
	figures/start-context-menu-nautilus.png			\
	figures/start-zenity-result.png					\
	figures/stock-icon-about.png

#DOC_LINGUAS = $(patsubst $(srcdir)/%,%,$(shell find $(srcdir) -mindepth 1 -maxdepth 1 -type d | $(GREP) -v $(srcdir)/C))
DOC_LINGUAS = de el es fr sl
