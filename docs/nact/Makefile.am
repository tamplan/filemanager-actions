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
#
# from Migrating your documentation to gnome-doc-utils
# http://live.gnome.org/GnomeDocUtilsMigrationHowTo
#
# do not use the $(NULL) syntax here as this may prevent Damned-Lies
# to correctly generate POT files (cf. mail of Claude Paroz 2010-08-29
# http://mail.gnome.org/archives/gnome-i18n/2010-August/msg00229.html).
#
# This produces the Nautilus-Actions Configuration Tool (NACT) Users's Manual.
# As a user's manual, it may be translated.
# Default is to produce DocBook XML documents, suitable for Yelp, the
# Gnome help system.
#
# --enable-html-manuals (resp. --enable-pdf-manuals) are configure options
# which produce HTML (resp. PDF) formats. These options are always enabled
# when making distcheck, so that all output formats for all known locales
# are always included in the distributed tarball.
#
# Note that gnome-doc-tool generates and distributes the translated xml
# files in docs/nact/<lang>/. When installing, g-d-t installs these help
# files in <datadir>/gnome/help/nautilus-actions-config-tool/<lang>.
# At install time, missing tranlated images are replace with symlinks to
# ../../C/figures/<image.png>.

include $(top_srcdir)/gnome-doc-utils.make

DOC_MODULE = nautilus-actions-config-tool

DOC_ENTITIES = $(patsubst C/%,%,$(shell \ls -1 C/*.xml | $(GREP) -v $(DOC_MODULE)))

DOC_INCLUDES =

DOC_FIGURES = $(patsubst C/%,%,$(shell \ls -1 C/figures/*)))

DOC_LINGUAS = de es sl

pkgdocdir = $(datarootdir)/doc/@PACKAGE@-@VERSION@

nactdir := docs/nact

all-local: all-manuals

.PHONY: all-manuals

all-manuals: all-html-manuals all-pdf-manuals

install-data-local: install-manuals

install-manuals: install-html-manuals install-pdf-manuals

uninstall-local: uninstall-manuals

uninstall-manuals: uninstall-html-manuals uninstall-pdf-manuals

dist-hook: doc-dist-hook dist-html-manuals-hook dist-pdf-manuals-hook dist-manuals-hooks

# Build HTML manuals as <lang>/html/<module>.html
#
# We have two alternative for generating HTML manual:
# - gnome-doc-tool (from gnome-doc-utils package)
# - db2html (from docbook-utils package)
# The actual tool to be used may be specified as an argument of 
# --enable-html-manuals configure option. It defaults to gnome-doc-tool.
#
# gnome-doc-tool <format> -o <dir> -d0 nautilus-actions-config-tool.xml
# - format: output format (html)
# - dir: if this does not name an existing directory in which *.html
#   files will be created, then is considered as the output filename
# -d0: nochunks
# The produced output is very close of what Yelp gives, with in particular
# a summary in each chapter while db2html does not produce them.
# As of v 0.20.1:
# - there is no icon for notes and tips (see #636175).
# - does not produce the legal and abstract sections in top of the document
# - does not honor external links: they are converted to simple links
#   which target the same frame window.
# gnome-doc-tool is so our preferred tool for its proximity with Yelp
# and the summary in top of the chapters.
#
# db2html --nochunks nautilus-actions-config-tool.xml
# Generate the html document(s) in a nautilus-actions-config-tool/
# subdirectory. 
# As of 0.6.14:
# - creates a stylesheet-images subdirectory
# - does not honor external links: they are converted to simple links
#   which target the same frame window.

if ENABLE_HTML_MANUALS
all-html-manuals: html.stamp
else
all-html-manuals:
endif

# _DOC_LC_DOCS is the list of the translated (not C) DOC_MODULE.xml files
# we so are sure that they exist and are up to date
# (path is .po ->[msgfmt]-> .mo ->[xml2po]-> .xml)
#
# - do not use '--copy_graphics' here (see #636175)
# - admon-* icons will be installed in admon/ subdir
html.stamp: $(_DOC_LC_DOCS)
	@-for i in C $(_DOC_REAL_LINGUAS); do \
		(cd $$i; \
		 if test "x$(WITH_DB2HTML)" = "xyes"; then \
			echo "Generating $$i/html/$(DOC_MODULE).html ..."; \
			rm -fr $(DOC_MODULE).junk; \
			rm -fr html; \
			cmd="db2html --nochunks $(DOC_MODULE).xml"; \
			echo "$$cmd"; \
			$$cmd; \
			mv -v $(DOC_MODULE) html; \
		 fi; \
		 if test "x$(WITH_GDT)" = "xyes"; then \
			echo "Generating $$i/html/$(DOC_MODULE).html ..."; \
			rm -fr html; \
		 	$(MKDIR_P) html; \
			cmd="gnome-doc-tool html -o html -d0 $(DOC_MODULE).xml"; \
			echo "$$cmd"; \
			$$cmd; \
			sed -i -e 's?url("admon?url("admon/admon?' html/$(DOC_MODULE).html; \
		 fi; \
		 chmod -R u+w html; \
		); \
	done
	touch html.stamp

# Install HTML manuals if they exist (do not try to rebuild them)
# in <htmldir>/<lang>/nact/ subdirectory; htmldir defaulting
# to <pkgdocdir>/html
#
# Note that we do not honor here the --with-html-dir gtk-doc option,
# but the --htmldir autotools one
install-html-manuals: all-html-manuals install-html-figs install-html-admon
	for lc in C $(_DOC_REAL_LINGUAS); do \
		if test "x$(htmldir)" = "x$(docdir)"; then \
			_instdir="$(DESTDIR)$(pkgdocdir)/html/$$lc/nact"; \
		else \
			_instdir="$(DESTDIR)$(htmldir)/$$lc/nact"; \
		fi; \
		_srcdir="$(top_builddir)/$(nactdir)/$$lc/html"; \
		echo "_srcdir=$$_srcdir"; \
		if test -f "$$_srcdir/$(DOC_MODULE).html"; then \
			cp -vrp $$_srcdir/* $$_instdir/; \
		fi; \
	done

# copied from gnome-doc-utils.make install-doc-figs target
# modified to install the figures with html manuals
install-html-figs:
	if test "x$(htmldir)" = "x$(docdir)"; then \
		_instdir="$(DESTDIR)$(pkgdocdir)/html"; \
	else \
		_instdir="$(DESTDIR)$(htmldir)"; \
	fi; \
	_srcdir="$(top_srcdir)/$(nactdir)"; \
	list='$(patsubst C/%,%,$(_DOC_C_FIGURES))'; \
	for lc in C $(_DOC_REAL_LINGUAS); do \
	  if test -f "$(top_builddir)/$(nactdir)/$$lc/html/$(DOC_MODULE).html"; then \
		for fig in $$list; do \
			figsymlink=false; \
			if test -f "$$_srcdir/$$lc/$$fig" -a ! -h "$$_srcdir/$$lc/$$fig"; then \
				figfile="$$_srcdir/$$lc/$$fig"; \
			else \
				figsymlink=true; \
			fi; \
			figdir=`echo $$fig | sed -e 's/^\(.*\/\).*/\1/' -e '/\//!s/.*//'`; \
			figdir="$$_instdir/$$lc/nact/$$figdir"; \
			if ! test -d "$$figdir"; then echo "$(mkinstalldirs) $$figdir"; $(mkinstalldirs) "$$figdir"; fi; \
			if $$figsymlink; then \
				cmd="cd $$figdir && $(LN_S) -f ../../../C/nact/$$fig"; \
			else \
				cmd="$(INSTALL_DATA) $$figfile $$figdir/"; \
			fi; \
			echo $$cmd; ( eval $$cmd ); \
		done; \
	  fi; \
	done

# A workaround against the bug #636175 (do not install admon-* icons)
# Only works when gnome-doc-utils is installed on the station
# Install icons in the html/ dir
admon_dir := /usr/share/gnome-doc-utils/icons/hicolor/48x48/status
admon_png := $(wildcard $(admon_dir)/*.png)
install-html-admon:
	if test -d "$(admon_dir)"; then \
		if test "x$(htmldir)" = "x$(docdir)"; then \
			_instdir="$(DESTDIR)$(pkgdocdir)/html"; \
		else \
			_instdir="$(DESTDIR)$(htmldir)"; \
		fi; \
		for lc in C $(_DOC_REAL_LINGUAS); do \
			if test -f "$(top_builddir)/$(nactdir)/$$lc/html/$(DOC_MODULE).html"; then \
				$(mkinstalldirs) "$$_instdir/$$lc/nact/admon"; \
				cp -v $(admon_png) "$$_instdir/$$lc/nact/admon/"; \
			fi; \
		done; \
	fi

uninstall-html-manuals:
	if test "x$(htmldir)" = "x$(docdir)"; then \
		_instdir="$(DESTDIR)$(pkgdocdir)/html"; \
	else \
		_instdir="$(DESTDIR)$(htmldir)"; \
	fi; \
	rm -fr $$_instdir

# locale figures (_DOC_LC_FIGURES) are distributed by gnome-doc-utils
# when they exist
#
# - distribute admon-* icons
dist-html-manuals-hook: all-html-manuals
	for lc in C $(_DOC_REAL_LINGUAS); do \
		if test -f $(top_builddir)/$(nactdir)/$$lc/html/$(DOC_MODULE).html; then \
			cp -rpv $(top_builddir)/$(nactdir)/$$lc/html $(distdir)/$$lc/; \
		fi; \
		if test -d "$(admon_dir)"; then \
			$(mkinstalldirs) "$(distdir)/$$lc/html/admon"; \
			cp -v $(admon_png) "$(distdir)/$$lc/html/admon/"; \
		fi; \
	done

# Build pdf manuals as <lang>/<module>.pdf
#
# We only use dblatex for now

if ENABLE_PDF_MANUALS
all-pdf-manuals: pdf.stamp
else
all-pdf-manuals:
endif

# _DOC_LC_DOCS is the list of the translated (not C) DOC_MODULE.xml files
# we so are sure that they exist and are up to date
# (path is .po ->[msgfmt]-> .mo ->[xml2po]-> .xml)
pdf.stamp: $(_DOC_LC_DOCS)
	@-for lc in C $(_DOC_REAL_LINGUAS); do \
		_target=$$lc/$(DOC_MODULE).pdf; \
		echo "Generating $$_target ..."; \
		if [ "$$lc" != "C" ]; then \
		  list='$(patsubst C/%,%,$(_DOC_C_FIGURES))'; \
		  for fig in $$list; do \
	        figdir="$$lc/"`echo $$fig | sed -e 's/^\(.*\/\).*/\1/' -e '/\//!s/.*//'`; \
	        figdir="$(builddir)/$$figdir"; \
	        if ! test -d "$$figdir"; then echo "$(MKDIR_P) $$figdir"; $(MKDIR_P) "$$figdir"; fi; \
	        figbase=`echo $$fig | sed -e 's/^.*\///'`; \
	        if ! test -f "$$lc/$$fig"; then \
	          echo "cd $$figdir && $(LN_S) -f ../../C/$$fig $$figbase"; \
	          ( cd "$$figdir" && $(LN_S) -f "../../C/$$fig" "$$figbase" ); \
	        fi; \
	      done; \
	    fi; \
		echo "dblatex --output $$_target $(top_srcdir)/$(nactdir)/$$lc/$(DOC_MODULE).xml"; \
		dblatex --output $$_target $(top_srcdir)/$(nactdir)/$$lc/$(DOC_MODULE).xml; \
		if [ "$$lc" != "C" ]; then \
		  for i in `find $$lc/figures -type l`; do rm -f $$i; done; \
		fi; \
	done
	touch pdf.stamp

# if --pdfdir has not been specified, then we default to PKGDOCDIR/pdf
# in all case, we append the locale as a subdirectory level
#
# as pdf are monobloc documents, we do not create a per-document subdir
install-pdf-manuals: all-pdf-manuals
	@-for lc in C $(_DOC_REAL_LINGUAS); do \
		_srcdir=$(top_builddir)/$(nactdir)/$$lc; \
		if test "$(pdfdir)" = "$(docdir)"; then \
			_instdir=$(DESTDIR)$(pkgdocdir)/pdf/$$lc; \
		else \
			_instdir=$(DESTDIR)$(pdfdir)/$$lc; \
		fi; \
		if ! test -d $$_instdir; then $(MKDIR_P) $$_instdir; fi; \
		echo $(INSTALL_DATA) "$$_srcdir/$(DOC_MODULE).pdf" "$$_instdir/"; \
		$(INSTALL_DATA) "$$_srcdir/$(DOC_MODULE).pdf" "$$_instdir/"; \
	done

uninstall-pdf-manuals:
	if test "$(pdfdir)" = "$(docdir)"; then \
		_instdir=$(DESTDIR)$(pkgdocdir)/pdf; \
	else \
		_instdir=$(DESTDIR)$(pdfdir); \
	fi; \
	rm -fr $$_instdir

dist-pdf-manuals-hook: all-pdf-manuals
	for _lc in C $(_DOC_REAL_LINGUAS); do \
		cp -v $(top_builddir)/$(nactdir)/$$_lc/$(DOC_MODULE).pdf $(distdir)/$$_lc/ || :; \
	done

dist-manuals-hooks:
	find $(distdir) -type d -exec chmod 755 {} \;
	find $(distdir) -type f -exec chmod 644 {} \;

# According to GNU Make manual (§ 14.6 Standard Targets for Users),
# clean (and distclean) should not remove distributed files, but
# make distcheck does not 'know' that we have dynamic extra dist
# (these manuals)
clean-local: local-clean-html local-clean-pdf local-clean-figs
	rm -f *~

distclean-local: local-clean-figs
	rm -f *.stamp

local-clean-local: local-clean-html local-clean-pdf local-clean-figs

local-clean-html:
	for lc in C $(_DOC_REAL_LINGUAS); do rm -fr $$lc/html; done

local-clean-pdf:
	for lc in C $(_DOC_REAL_LINGUAS); do rm -f $$lc/*.pdf; done

local-clean-figs:
	for lc in $(_DOC_REAL_LINGUAS); do \
		if test -d $$lc/figures; then \
			for l in `find $$lc/figures -type l`; do rm -vf $$l; done; \
			(rmdir $$lc/figures 2>/dev/null && echo "rmdir $$lc/figures"); \
		fi; \
	done
