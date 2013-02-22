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

NA_UTILS_LDADD = \
	$(top_builddir)/src/core/libna-core.la						\
	$(NAUTILUS_ACTIONS_LIBS)									\
	$(NULL)

bin_PROGRAMS = \
	nautilus-actions-new										\
	nautilus-actions-print										\
	nautilus-actions-run										\
	$(NULL)

pkglibexec_PROGRAMS = \
	na-print-schemas											\
	na-set-conf													\
	$(NULL)

AM_CPPFLAGS += \
	-I $(top_srcdir)											\
	-I $(top_srcdir)/src										\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"					\
	-DG_LOG_DOMAIN=\"${NA_LOGDOMAIN_UTILS}\"					\
	$(NAUTILUS_ACTIONS_CFLAGS)									\
	$(NULL)

nautilus_actions_new_SOURCES = \
	nautilus-actions-new.c										\
	console-utils.c												\
	console-utils.h												\
	$(NULL)

nautilus_actions_new_LDADD = \
	$(NA_UTILS_LDADD)											\
	$(NULL)

nautilus_actions_print_SOURCES = \
	nautilus-actions-print.c									\
	console-utils.c												\
	console-utils.h												\
	$(NULL)

nautilus_actions_print_LDADD = \
	$(NA_UTILS_LDADD)											\
	$(NULL)

BUILT_SOURCES =

if HAVE_GDBUS
BUILT_SOURCES += \
	nautilus-actions-run-bindings.c								\
	nautilus-actions-run-bindings.h								\
	$(NULL)

nautilus-actions-run-bindings.c nautilus-actions-run-bindings.h: $(top_srcdir)/src/plugin-tracker/na-tracker-gdbus.xml
	gdbus-codegen \
		--interface-prefix org.nautilus_actions.DBus.Tracker.	\
		--generate-c-code nautilus-actions-run-bindings			\
		--c-namespace NA_Tracker								\
		--c-generate-object-manager								\
		$<
endif

if HAVE_DBUS_GLIB
BUILT_SOURCES += \
	nautilus-actions-run-bindings.h								\
	$(NULL)

nautilus-actions-run-bindings.h: $(top_srcdir)/src/plugin-tracker/na-tracker-dbus-glib.xml
	dbus-binding-tool --mode=glib-client $< > $@
endif

nodist_nautilus_actions_run_SOURCES = \
	$(BUILT_SOURCES)											\
	$(NULL)

nautilus_actions_run_SOURCES = \
	nautilus-actions-run.c										\
	console-utils.c												\
	console-utils.h												\
	$(NULL)

nautilus_actions_run_LDADD = \
	$(NA_UTILS_LDADD)											\
	$(NULL)

na_print_schemas_SOURCES = \
	na-print-schemas.c											\
	console-utils.c												\
	console-utils.h												\
	$(NULL)

na_print_schemas_LDADD = \
	$(NA_UTILS_LDADD)											\
	$(NULL)

na_set_conf_SOURCES = \
	na-set-conf.c												\
	console-utils.c												\
	console-utils.h												\
	$(NULL)

na_set_conf_LDADD = \
	$(NA_UTILS_LDADD)											\
	$(NULL)

EXTRA_DIST = \
	na-gconf2key.sh.in											\
	$(NULL)

CLEANFILES = \
	$(BUILT_SOURCES)											\
	$(NULL)

# If GConf support is enabled, then also build the migration tools

if HAVE_GCONF
pkglibexec_PROGRAMS += \
	na-delete-xmltree											\
	$(NULL)

na_delete_xmltree_SOURCES = \
	na-delete-xmltree.c											\
	console-utils.c												\
	console-utils.h												\
	$(NULL)

na_delete_xmltree_LDADD = \
	$(NA_UTILS_LDADD)											\
	$(NULL)

pkglibexec_SCRIPTS = \
	na-gconf2key.sh												\
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
