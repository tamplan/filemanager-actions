# Nautilus-Actions
# A Nautilus extension which offers configurable context menu actions.
#
# Copyright (C) 2005 The GNOME Foundation
# Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
# Copyright (C) 2009, 2010, 2011 Pierre Wieser and others (see AUTHORS)
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

# Test programs are only relevant for the maintainer and his own tests
# Do not build them in a release

if NA_MAINTAINER_MODE

noinst_PROGRAMS = \
	test-reader											\
	test-iface											\
	test-parse-uris										\
	test-virtuals										\
	test-virtuals-without-test							\
	$(NULL)

AM_CPPFLAGS += \
	-I $(top_srcdir)									\
	-I $(top_srcdir)/src								\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"			\
	-DPKGLIBDIR=\""$(pkglibdir)"\"						\
	-DG_LOG_DOMAIN=\"${NA_LOGDOMAIN_TEST}\"				\
	$(NAUTILUS_ACTIONS_CFLAGS)							\
	$(NULL)

test_reader_SOURCES = \
	test-reader.c										\
	$(NULL)

test_reader_LDADD = \
	$(top_builddir)/src/core/libna-core.la				\
	$(NAUTILUS_ACTIONS_LIBS)							\
	$(NULL)

test_iface_SOURCES = \
	test-iface.c										\
	test-iface-iface.c									\
	test-iface-iface.h									\
	test-iface-base.c									\
	test-iface-base.h									\
	test-iface-derived.c								\
	test-iface-derived.h								\
	$(NULL)

test_iface_LDADD = \
	$(NAUTILUS_ACTIONS_LIBS)							\
	$(NULL)

if NA_MAINTAINER_MODE
noinst_PROGRAMS += test-module
pkglib_LTLIBRARIES = libtest_module_plugin.la

test_module_SOURCES = \
	test-module.c										\
	$(NULL)

test_module_LDADD = \
	$(NAUTILUS_ACTIONS_LIBS)							\
	$(NULL)

libtest_module_plugin_la_SOURCES = \
	test-module-plugin.c								\
	$(NULL)

libtest_module_plugin_la_LIBADD = \
	$(NAUTILUS_ACTIONS_LIBS)							\
	$(NULL)

libtest_module_plugin_la_LDFLAGS = \
	-module 											\
	-no-undefined										\
	-avoid-version										\
	$(NULL)
endif

test_parse_uris_SOURCES = \
	test-parse-uris.c									\
	$(NULL)

test_parse_uris_LDADD = \
	$(top_builddir)/src/core/libna-core.la				\
	$(NAUTILUS_ACTIONS_LIBS)							\
	$(NULL)

test_virtuals_SOURCES = \
	test-virtuals.c										\
	$(NULL)

test_virtuals_LDADD = \
	$(NAUTILUS_ACTIONS_LIBS)							\
	$(NULL)

test_virtuals_without_test_SOURCES = \
	test-virtuals-without-test.c						\
	$(NULL)

test_virtuals_without_test_LDADD = \
	$(NAUTILUS_ACTIONS_LIBS)							\
	$(NULL)

endif
#if NA_MAINTAINER_MODE
