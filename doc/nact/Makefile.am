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

# from Migrating your documentation to gnome-doc-utils
# http://live.gnome.org/GnomeDocUtilsMigrationHowTo

include $(top_srcdir)/gnome-doc-utils.make

dist-hook: doc-dist-hook

DOC_MODULE = nautilus-actions-config-tool

DOC_ENTITIES = \
	appendix-gnu-fdl-1.3.xml						\
	articleinfo.xml									\
	legalnotice.xml									\
	nact-bugs.xml									\
	nact-conditions.xml								\
	nact-edit.xml									\
	nact-execution.xml								\
	nact-export.xml									\
	nact-import.xml									\
	nact-menus.xml									\
	nact-misc.xml									\
	nact-prefs.xml									\
	nact-profiles.xml								\
	nact-save.xml									\
	nact-sorting.xml								\
	nact-starting.xml								\
	nact-where.xml									\
	$(NULL)

DOC_INCLUDES = \
	$(NULL)

DOC_FIGURES = \
	figures/add.png									\
	figures/nact-action-tab.png						\
	figures/nact-basenames-tab.png					\
	figures/nact-capabilities-tab.png				\
	figures/nact-command-tab.png					\
	figures/nact-desktop-environment-frame.png		\
	figures/nact-environment-tab.png				\
	figures/nact-exec-mode-frame.png				\
	figures/nact-execute-as-frame.png				\
	figures/nact-execution-environment-frame.png	\
	figures/nact-execution-tab.png					\
	figures/nact-folders-tab.png					\
	figures/nact-legend.png							\
	figures/nact-main-empty.png						\
	figures/nact-mimetypes-tab.png					\
	figures/nact-new-capability.png					\
	figures/nact-new-scheme.png						\
	figures/nact-preferences-export.png				\
	figures/nact-preferences-import.png				\
	figures/nact-preferences-io-providers.png		\
	figures/nact-preferences-runtime.png			\
	figures/nact-preferences-schemes.png			\
	figures/nact-preferences-ui.png					\
	figures/nact-properties-tab.png					\
	figures/nact-schemes-tab.png					\
	figures/nact-selection-count-frame.png			\
	figures/nact-startup-mode-frame.png				\
	figures/nact-target-location.png				\
	figures/nact-working-directory-frame.png		\
	figures/remove.png								\
	figures/sort-asc.png							\
	figures/sort-desc.png							\
	figures/sort-manual.png							\
	figures/start-context-menu.png					\
	figures/start-context-menu-nautilus.png			\
	figures/start-zenity-result.png					\
	$(NULL)

DOC_LINGUAS = \
	$(NULL)

# generate HTML and PDF NACT user's manual if --enable-manuals has been set
all-local: manuals

manuals: html-manual pdf-manual

sources_dir = $(top_srcdir)/doc/nact/C

target_dir = $(top_builddir)/doc/nact/C

# let DOC_MODULE.xml be the first of the list
sources_files = \
	$(addprefix 									\
		$(sources_dir)/, 							\
			$(DOC_MODULE).xml						\
			$(DOC_ENTITIES)							\
			$(DOC_INCLUDES)							\
			$(DOC_FIGURES))							\
	$(NULL)

$(sources_files):

target_html = $(target_dir)/$(DOC_MODULE)/$(DOC_MODULE).html

target_pdf = $(target_dir)/$(DOC_MODULE).pdf

if ENABLE_HTML_MANUALS
html-manual: $(target_html)

$(target_html): $(sources_files)
	@echo "Generating $(target_html)..."
	-db2html -b html -u $<
else
html-manual:
endif

if ENABLE_PDF_MANUALS
pdf-manual: $(target_pdf)

$(target_pdf): $(sources_files)
	@echo "Generating $(target_pdf)..."
	-dblatex $< >/dev/null
else
pdf-manual:
endif

clean-local:
	rm -f *~
	rm -f  $(target_dir)/*.pdf
	rm -f  $(target_dir)/*.html
	rm -fr $(target_dir)/$(DOC_MODULE)
