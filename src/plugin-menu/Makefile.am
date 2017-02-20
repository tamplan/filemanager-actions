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
	-I $(top_srcdir)									\
	-I $(top_srcdir)/src								\
	-DG_LOG_DOMAIN=\"FMA\"								\
	$(NAUTILUS_ACTIONS_CFLAGS)							\
	$(CODE_COVERAGE_CFLAGS)								\
	$(NULL)

lib_sources = \
	fma-menu-module.c									\
	fma-menu-plugin.c									\
	fma-menu-plugin.h									\
	$(NULL)

lib_libadd = \
	$(top_builddir)/src/core/libfma-core.la				\
	$(NAUTILUS_ACTIONS_LIBS)							\
	$(NULL)

lib_ldflags = \
	-module 											\
	-no-undefined										\
	-avoid-version										\
	$(CODE_COVERAGE_LDFLAGS)							\
	$(NULL)

if HAVE_NAUTILUS
nautilus_extensiondir = $(NAUTILUS_EXTENSIONS_DIR)
nautilus_extension_LTLIBRARIES = libfma-nautilus-menu.la
libfma_nautilus_menu_la_SOURCES = $(lib_sources)
libfma_nautilus_menu_la_LIBADD = $(lib_libadd)
libfma_nautilus_menu_la_LDFLAGS = $(lib_ldflags)
libfma_nautilus_menu_la_CPPFLAGS = $(AM_CPPFLAGS)		\
	-DFMA_TARGET_ID=$(NAUTILUS_ID)						\
	-DFMA_TARGET_LABEL=\"$(NAUTILUS_LABEL)\"			\
	$(NULL)
endif

if HAVE_NEMO
nemo_extensiondir = $(NEMO_EXTENSIONS_DIR)
nemo_extension_LTLIBRARIES = libfma-nemo-menu.la
libfma_nemo_menu_la_SOURCES = $(lib_sources)
libfma_nemo_menu_la_LIBADD = $(lib_libadd)
libfma_nemo_menu_la_LDFLAGS = $(lib_ldflags)
libfma_nemo_menu_la_CPPFLAGS = $(AM_CPPFLAGS) 			\
	-DFMA_TARGET_ID=$(NEMO_ID)							\
	-DFMA_TARGET_LABEL=\"$(NEMO_LABEL)\"				\
	$(NULL)
endif

if HAVE_CAJA
caja_extensiondir = $(CAJA_EXTENSIONS_DIR)
caja_extension_LTLIBRARIES = libfma-caja-menu.la
libfma_caja_menu_la_SOURCES = $(lib_sources)
libfma_caja_menu_la_LIBADD = $(lib_libadd)
libfma_caja_menu_la_LDFLAGS = $(lib_ldflags)
libfma_caja_menu_la_CPPFLAGS = $(AM_CPPFLAGS) 			\
	-DFMA_TARGET_ID=$(CAJA_ID)							\
	-DFMA_TARGET_LABEL=\"$(CAJA_LABEL)\"				\
	$(NULL)
endif

# Code coverage
@CODE_COVERAGE_RULES@
