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

schemas_in_files = \
	nautilus-actions-prefs.schemas.in					\
	nautilus-actions-confs.schemas.in					\
	$(NULL)

schemas_files = $(schemas_in_files:.schemas.in=.schemas)

nautilus-actions-confs.schemas.in:
	$(top_srcdir)/src/utils/nautilus-actions-schemas -s > $@

schemasdir = $(pkgdatadir)/gconf-schemas

schemas_DATA = $(schemas_files)

@INTLTOOL_SCHEMAS_RULE@

if GCONF_SCHEMAS_INSTALL
NA_INSTALL_SCHEMAS  = yes
else
NA_INSTALL_SCHEMAS = no
endif

EXTRA_DIST = \
	$(schemas_in_files)									\
	$(NULL)

install-data-hook:
	@if test "$(NA_INSTALL_SCHEMAS)" = "yes"; then \
		if test `id -u` != 0; then \
			echo "***"; \
			echo "*** You need to be root to install GConf schemas"; \
			echo "***"; \
			exit 1; \
		fi; \
		for p in $(schemas_DATA); do \
			GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $$p; \
		done \
	fi

CLEANFILES = $(schemas_files)

uninstall-hook:
	@if test "$(NA_INSTALL_SCHEMAS)" = "yes"; then \
		for p in $(schemas_DATA); do \
			GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-uninstall-rule $$p; \
		done \
	fi
