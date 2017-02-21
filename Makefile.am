# FileManager-Actions
# A file-manager extension which offers configurable context menu actions.
#
# Copyright (C) 2005 The GNOME Foundation
# Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
# Copyright (C) 2009-2014 Pierre Wieser and others (see AUTHORS)
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

ACLOCAL_AMFLAGS = -I m4

# src/ must be built before maintainer/
# docs/ must be built after src/
SUBDIRS = \
	data								\
	icons								\
	m4									\
	src									\
	docs								\
	maintainer							\
	po									\
	$(NULL)

# - nautilus-extdir is a fake directory so that make distcheck
#   has a vpath which is prefixed with a make variable, instead of an
#   hardcoded as returned by pkg-config
# - do not try to install schemas when making distcheck (defaults to be disabled)
# - do not rebuild fma-config-tool manuals (defaults to be disabled)
# - Debian 7 Wheezy requires gtk-doc to be enabled (v1.18)
#   while this is no more needed in Fedora 24 (v1.25)
DISTCHECK_CONFIGURE_FLAGS = \
	--with-nautilus-extdir='$${libdir}/nautilus/extensions-3.0-distcheck' \
	--enable-as-needed					\
	--enable-gtk-doc					\
	$(NULL)

EXTRA_DIST = \
	fma_required_versions				\
	$(NULL)

DISTCLEANFILES = \
	$(NULL)

pkgdocdir = $(datarootdir)/doc/@PACKAGE@-@VERSION@

dist_pkgdoc_DATA = \
	AUTHORS								\
	ChangeLog							\
	ChangeLog-2008						\
	ChangeLog-2009						\
	ChangeLog-2010						\
	ChangeLog-2011						\
	ChangeLog-2012						\
	ChangeLog-2014						\
	COPYING								\
	COPYING-DOCS						\
	INSTALL								\
	MAINTAINERS							\
	NEWS								\
	README								\
	README-GCONF						\
	TODO								\
	$(NULL)

uninstall-hook:
	-\rm -fr $(DESTDIR)$(pkgdocdir)
