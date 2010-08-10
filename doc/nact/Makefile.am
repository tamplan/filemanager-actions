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
	appendix-gnu-fdl-1.3.xml					\
	articleinfo.xml								\
	legalnotice.xml								\
	nact-bugs.xml								\
	nact-conditions.xml							\
	nact-edit.xml								\
	nact-execution.xml							\
	nact-export.xml								\
	nact-import.xml								\
	nact-menus.xml								\
	nact-misc.xml								\
	nact-prefs.xml								\
	nact-profiles.xml							\
	nact-save.xml								\
	nact-sorting.xml							\
	nact-starting.xml							\
	nact-where.xml								\
	$(NULL)

DOC_INCLUDES = \
	$(NULL)

DOC_FIGURES = \
	figures/add.png								\
	figures/remove.png							\
	figures/nact-action-tab.png					\
	figures/nact-basenames-tab.png				\
	figures/nact-capabilities-tab.png			\
	figures/nact-command-tab.png				\
	figures/nact-environment-tab.png			\
	figures/nact-execution-tab.png				\
	figures/nact-folders-tab.png				\
	figures/nact-legend.png						\
	figures/nact-main-empty.png					\
	figures/nact-mimetypes-tab.png				\
	figures/nact-new-capability.png				\
	figures/nact-new-scheme.png					\
	figures/nact-preferences-export.png			\
	figures/nact-preferences-import.png			\
	figures/nact-preferences-io-providers.png	\
	figures/nact-preferences-runtime.png		\
	figures/nact-preferences-schemes.png		\
	figures/nact-preferences-ui.png				\
	figures/nact-properties-tab.png				\
	figures/nact-schemes-tab.png				\
	figures/start-context-menu.png				\
	figures/start-context-menu-nautilus.png		\
	figures/start-zenity-result.png				\
	$(NULL)

DOC_LINGUAS = \
	$(NULL)

clean-local :
	rm -f *~
