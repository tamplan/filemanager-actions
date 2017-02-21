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

if HAVE_GCONF

schemas_in_files = \
	nautilus-actions-prefs.schemas.in					\
	nautilus-actions-confs.schemas.in					\
	$(NULL)

schemas_files = $(schemas_in_files:.schemas.in=.schemas)

nautilus-actions-confs.schemas.in:
	$(top_srcdir)/src/utils/fma-print-schemas -s > $@

schemasdir = $(pkgdatadir)/gconf-schemas

schemas_DATA = $(schemas_files)

@INTLTOOL_SCHEMAS_RULE@

if GCONF_SCHEMAS_INSTALL
FMA_INSTALL_SCHEMAS = yes
else
FMA_INSTALL_SCHEMAS = no
endif

EXTRA_DIST = \
	$(schemas_in_files)									\
	$(NULL)

install-data-hook:
	@if test "$(FMA_INSTALL_SCHEMAS)" = "yes"; then \
		if test `id -u` != 0; then \
			echo "GConf schemas will not be installed because you are not root"; \
		else \
			for p in $(schemas_DATA); do \
				GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $$p; \
			done \
		fi; \
	fi

CLEANFILES = $(schemas_files)

uninstall-hook:
	@if test "$(FMA_INSTALL_SCHEMAS)" = "yes"; then \
		for p in $(schemas_DATA); do \
			GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-uninstall-rule $$p; \
		done \
	fi

endif
