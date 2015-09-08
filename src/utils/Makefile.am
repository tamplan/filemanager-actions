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

NA_UTILS_LDADD = \
	$(top_builddir)/src/core/libfma-core.la						\
	$(NAUTILUS_ACTIONS_LIBS)									\
	$(NULL)

pkglibexec_PROGRAMS = \
	fma-new														\
	fma-print													\
	fma-print-schemas											\
	fma-run														\
	fma-set-conf												\
	$(NULL)

AM_CPPFLAGS += \
	-I $(top_srcdir)											\
	-I $(top_srcdir)/src										\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"					\
	-DG_LOG_DOMAIN=\"${NA_LOGDOMAIN_UTILS}\"					\
	$(NAUTILUS_ACTIONS_CFLAGS)									\
	$(NULL)

fma_new_SOURCES = \
	fma-new.c													\
	console-utils.c												\
	console-utils.h												\
	$(NULL)

fma_new_LDADD = \
	$(NA_UTILS_LDADD)											\
	$(NULL)

fma_print_SOURCES = \
	fma-print.c													\
	console-utils.c												\
	console-utils.h												\
	$(NULL)

fma_print_LDADD = \
	$(NA_UTILS_LDADD)											\
	$(NULL)

BUILT_SOURCES =

BUILT_SOURCES += \
	fma-run-bindings.c											\
	fma-run-bindings.h											\
	$(NULL)

fma-run-bindings.c fma-run-bindings.h: $(top_srcdir)/src/plugin-tracker/fma-tracker-gdbus.xml
	gdbus-codegen \
		--interface-prefix org.filemanager_actions.DBus.Tracker.	\
		--generate-c-code fma-run-bindings						\
		--c-namespace FMA_Tracker_GDBus							\
		--c-generate-object-manager								\
		$<

nodist_fma_run_SOURCES = \
	$(BUILT_SOURCES)											\
	$(NULL)

fma_run_SOURCES = \
	fma-run.c										\
	console-utils.c												\
	console-utils.h												\
	$(NULL)

fma_run_LDADD = \
	$(NA_UTILS_LDADD)											\
	$(NULL)

fma_print_schemas_SOURCES = \
	fma-print-schemas.c											\
	console-utils.c												\
	console-utils.h												\
	$(NULL)

fma_print_schemas_LDADD = \
	$(NA_UTILS_LDADD)											\
	$(NULL)

fma_set_conf_SOURCES = \
	fma-set-conf.c												\
	console-utils.c												\
	console-utils.h												\
	$(NULL)

fma_set_conf_LDADD = \
	$(NA_UTILS_LDADD)											\
	$(NULL)

EXTRA_DIST = \
	fma-gconf2key.sh.in											\
	$(NULL)

CLEANFILES = \
	$(BUILT_SOURCES)											\
	$(NULL)

# If GConf support is enabled, then also build the migration tools

if HAVE_GCONF
pkglibexec_PROGRAMS += \
	fma-delete-xmltree											\
	$(NULL)

fma_delete_xmltree_SOURCES = \
	fma-delete-xmltree.c										\
	console-utils.c												\
	console-utils.h												\
	$(NULL)

fma_delete_xmltree_LDADD = \
	$(NA_UTILS_LDADD)											\
	$(NULL)

pkglibexec_SCRIPTS = \
	fma-gconf2key.sh											\
	$(NULL)

do_subst = sed \
		-e 's,[@]PACKAGE[@],$(PACKAGE),g'						\
		-e 's,[@]PACKAGE_NAME[@],$(PACKAGE_NAME),g'				\
		-e 's,[@]PACKAGE_VERSION[@],$(PACKAGE_VERSION),g'		\
		-e 's,[@]sysconfdir[@],$(sysconfdir),g'					\
		-e 's,[@]bindir[@],$(bindir),g'							\
		-e 's,[@]libexecdir[@],$(libexecdir),g'					\
		-e 's,[@]pkglibexecdir[@],$(pkglibexecdir),g'			\
		$(NULL)

%.sh: %.sh.in
	$(do_subst) < $< > $@
	chmod a+x $@

CLEANFILES += \
	$(pkglibexec_SCRIPTS)										\
	$(NULL)
endif
