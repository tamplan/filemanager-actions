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

# pwi 2012-10-12
# Starting with NA 3.2.3, we are using this hacked version of
# gnome-doc-utils.make rather than the standard one.
#
# Rationale:
# - I, as a maitainer, have chosen to build myself and distribute all
#   manuals and documentation, so preventing the user or the packager
#   which would build from the tarball, to have the pain to install all
#   the documentation machinery (gnome-doc-utils, gtk-doc, dblatex, and
#   so on)
# - I think this is a rather rationale choice, as these files are
#   nonetheless architecture-independant data, and can be so safey used
#   as is by users and packagers
#
# Maybe I am not the only maintainer which have made this choice..
#
# But:
# - when building from sources, build as usually if new file exists
# - when building from tarball, does not build
#   this basically means that built files must be installed from the
#   dist to the ad-hoc place in builddir/ tree
# - when installing from sources,
# - gnome-doc-utils.make does not have any conditionals which would
#   only *build* or only *install* the documents
# - this is so a pain for the maintainer to include gnome-doc-utils.make,
#   and then hack its Makefile to only build and dist, or build and install,
#   or just install from sources or from dist.
#
# Basically, we would need:
# a/ build in builddir
# b/ dist from srcdir (sources) and builddir (built docs)
# c/ install from srcdir (if dist) or from builddir (if built)
#
# But, as gnome-doc-utils is deprecated to the profit of yelp-tools,
# there is very little chance for the developper to accept a new RFE :(
# So this dirty hack!
#
# What is the hack ?
# Just distinguish between build and install phases, so that we are not
# required to have gnome-doc-utils when just building from sources.
# Note that this also distributing generated .omf files.
#
# pwi 2015- 9-10
# Starting with FMA 3.4, we let the gnome-doc-utils.make standard
# Makefile does it work. This gdu-hack subdirectory is just a way to
# have a chance to install the built documents found in the distributed
# tarball into the builddir, before the standard Makefile begins its work.
# The result is that the built is not needed (should not be needed).

DOC_MODULE = fma-config-tool

# if we find installable files in the srcdir, then we are building from
# a distributed tarball. So just touch built files to actually not run
# build tools
myall:
	manualsrcdir=`dirname $(srcdir)`; \
	manualsrcdir=`cd $$manualsrcdir; pwd`; \
	manualbuilddir=`cd $(builddir); pwd`; \
	manualbuilddir=`dirname $$manualbuilddir`; \
	langdirs=`find $$manualsrcdir -mindepth 1 -maxdepth 1 -type d | $(GREP) -vE '/C|/gdu-hack'`; \
	for langdir in $$langdirs; do \
		lang=`basename $$langdir`; \
		if [ -e $$langdir/$(DOC_MODULE).xml ]; then \
			$(MKDIR_P) $$manualbuilddir/$$lang; \
			touch $$manualbuilddir/$$lang/$$lang.mo; \
			cp $$langdir/$(DOC_MODULE).xml $$manualbuilddir/$$lang/; \
		fi; \
		if [ -e $$manualsrcdir/$(DOC_MODULE)-$$lang.omf ]; then \
			cp $$manualsrcdir/$(DOC_MODULE)-$$lang.omf $$manualbuilddir/; \
		fi; \
	done

all: myall

mydist:
	manualsrcdir=`dirname $(srcdir)`; \
	manualsrcdir=`cd $$manualsrcdir; pwd`; \
	manualbuilddir=`cd $(builddir); pwd`; \
	manualbuilddir=`dirname $$manualbuilddir`; \
	manualdistdir=`cd $(distdir); pwd`; \
	manualdistdir=`dirname $$manualdistdir`; \
	langdirs=`find $$manualsrcdir -mindepth 1 -maxdepth 1 -type d | $(GREP) -vE '/C|/gdu-hack'`; \
	for langdir in $$langdirs; do \
		lang=`basename $$langdir`; \
		$(MKDIR_P) $$manualdistdir/$$lang; \
		cp $$manualbuilddir/$$lang/$(DOC_MODULE).xml $$manualdistdir/$$lang/; \
		cp $$manualbuilddir/$(DOC_MODULE)-$$lang.omf $$manualdistdir/; \
	done

dist: mydist
