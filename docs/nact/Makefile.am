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
#
# Also, do not confuse gnome-doc-utils which targets the help documentation
# of the Gnome (i.e. gui) applications, and that we are using here to
# generate html and pdf NACT user's manuals, with gtk-doc which rather
# targets the developer documentation (see docs/reference).

if ENABLE_MANUALS

include $(top_srcdir)/gnome-doc-utils.make

DOC_MODULE = nautilus-actions-config-tool

DOC_ENTITIES = $(patsubst $(srcdir)/C/%,%,$(shell \ls -1 $(srcdir)/C/*.xml | $(GREP) -v $(DOC_MODULE)))
#DOC_ENTITIES = "appendix-gnu-fdl-1.3.xml articleinfo.xml ... nact-where.xml"

DOC_INCLUDES =

DOC_FIGURES = $(patsubst $(srcdir)/C/%,%,$(shell \ls -1 $(srcdir)/C/figures/*.png))
#DOC_FIGURES = "figures/add.png figures/nact-action-tab.png ... figures/stock-icon-about.png"

DOC_LINGUAS = de es fr sl

# _DOC_LC_DOCS is the list of the translated (not C) DOC_MODULE.xml files
# we so are sure that they exist and are up to date
# (path is .po ->[msgfmt]-> .mo ->[xml2po]-> .xml)
MANUALS_XML = C/$(DOC_MODULE).xml $(_DOC_LC_DOCS)

# just an empty value to be able to add something for html and pdf manuals
MAINTAINERCLEANFILES =

pkgdocdir = $(datarootdir)/doc/@PACKAGE@-@VERSION@

all-local: all-manuals all-html-manuals all-pdf-manuals

.PHONY: \
	all-manuals install-manuals uninstall-manuals dist-manuals-hook \
	all-html-manuals install-html-manuals install-html-figs install-html-admon \
	uninstall-html-manuals dist-html-manuals-hook dist-html-admon dist-html-doc \
	distclean-html maintainer-clean-html \
	all-pdf-manuals install-pdf-manuals \
	uninstall-pdf-manuals dist-pdf-manuals-hook \
	distclean-pdf maintainer-clean-pdf

all-manuals: all-html-manuals all-pdf-manuals

install-data-local: install-manuals

install-manuals: install-html-manuals install-pdf-manuals

uninstall-local: uninstall-manuals

uninstall-manuals: uninstall-html-manuals uninstall-pdf-manuals

# doc-dist-hook is the dist hook from gnome-doc-utils.make
# it distributes both C (original) and generated (translated) .xml files,
# but not generated .omf files which so will have to be re-generated from
# the distributed .omf.in file
dist-hook: doc-dist-hook dist-manuals-hook

dist-manuals-hook: dist-html-manuals-hook dist-pdf-manuals-hook

# Build HTML manuals as $(srcdir)/<lang>/<module>.html
#
# We have two alternative for generating HTML manual:
# - gnome-doc-tool (from gnome-doc-utils package)
# - db2html (from docbook-utils package)
# The actual tool to be used may be specified as an argument of 
# --enable-html-manuals configure option. It defaults to gnome-doc-tool.
#
# gnome-doc-tool <format> -o <dir> -d0 nautilus-actions-config-tool.xml
# - format: output format (html)
# - dir: output directory
# -d0: nochunks
# The produced output is very close of what Yelp gives, with in particular
# a summary in each chapter while db2html does not produce it.
# As of v 0.20.1:
# - there is no icon for notes and tips (see #636175).
# - does not produce the legal and abstract sections in top of the document
# - does not honor external links: they are converted to simple links
#   which target the same frame window.
# gnome-doc-tool is so our preferred tool for its proximity with Yelp
# and the summary in top of the chapters.
#
# db2html --nochunks nautilus-actions-config-tool.xml
# Generate the html document(s). 
# As of 0.6.14:
# - does produce the legal and abstract sections in top of the document
# - creates a nautilus-actions-config-tool/stylesheet-images subdirectory
# - does not honor external links: they are converted to simple links
#   which target the same frame window
# - may create docbook2html-dir(.junk) subdirectories

MANUALS_HTML = $(MANUALS_XML:%.xml=%.html)

MAINTAINERCLEANFILES += $(MANUALS_HTML)

if ENABLE_HTML_MANUALS
all-html-manuals: $(MANUALS_HTML)
else
all-html-manuals:
endif

# - do not use '--copy_graphics' here (see #636175)
# - admon-* icons will be installed in admon/ subdir
#
# The rule here is: only rebuild html/ files if corresponding xml has changed
# .xml themselves are distributed, so are not modified when compiling from a
# distribution
#
# current=/home/pierre/data/eclipse/nautilus-actions/nautilus-actions-3.1.5/_build/docs/nact
# srcdir=../../../docs/nact
# top_srcdir=../../..
#
# db2html 0.6.14 bugs and work-arounds:
# - '*.junk' and 'docbook2html-dir/' are subdirectories left by db2html when
#   it is run without argument (see https://bugzilla.redhat.com/show_bug.cgi?id=756930)
# - stylesheet-images directory is not created in the output directory
#   (see https://bugzilla.redhat.com/show_bug.cgi?id=756928)
#
# gnome-doc-tool 0.20 bugs and work-arounds:
# - g-d-t does not honor the base directory of the source document, only
#   searching for extern entities in the current working directory
#   (see https://bugzilla.gnome.org/show_bug.cgi?id=664784)

$(MANUALS_HTML): %.html: %.xml
	@echo "Generating $@ ..."; \
	lc=$<; lc=$${lc%/*}; \
	if test "x$(WITH_DB2HTML)" = "xyes"; then \
		cmd="db2html --output $$lc --nochunks $<"; echo "$$cmd"; (eval $$cmd); \
	    (cd $$lc; mv -v $(DOC_MODULE)/stylesheet-images .; rmdir -v $(DOC_MODULE)); \
	    find . -type d -name '*.junk' | xargs rm -fr; \
	    find . -type d -name 'docbook2html-dir' | xargs rm -fr; \
	fi; \
	if test "x$(WITH_GDT)" = "xyes"; then \
		(cd $$lc; \
		 fhtml=$@; fhtml=$${fhtml##*/}; \
		 fxml=$<; fxml=$${fxml##*/}; \
		 cmd="gnome-doc-tool html -o . -d0 $$fxml"; echo "$$cmd"; (eval $$cmd); \
		 sed -i -e 's?url("admon?url("admon/admon?' $$fhtml; \
		); \
	fi

# Install HTML manuals if they exist (do not try to rebuild them)
# in <htmldir>/<lang>/nact/ subdirectory; htmldir defaulting
# to <pkgdocdir>/html
#
# Note that we do honor here --htmldir autotools configuration option.

if ENABLE_HTML_MANUALS
install-html-manuals: all-html-manuals install-html-figs install-html-admon
	for lc in C $(_DOC_REAL_LINGUAS); do \
		if test "x$(htmldir)" = "x$(docdir)"; then \
			_instdir="$(DESTDIR)$(pkgdocdir)/html/$$lc/nact"; \
		else \
			_instdir="$(DESTDIR)$(htmldir)/$$lc/nact"; \
		fi; \
		if test -f "$(srcdir)/$$lc/$(DOC_MODULE).html"; then \
			cp -vp "$(srcdir)/$$lc/$(DOC_MODULE).html" $$_instdir/; \
			if test -d "$(srcdir)/$$lc/stylesheet-images"; then \
				cmd="cp -vrp \"$(srcdir)/$$lc/stylesheet-images\" $$_instdir/"; echo $$cmd; (eval $$cmd); \
			fi; \
		fi; \
	done
else
install-html-manuals:
endif

# copied from gnome-doc-utils.make install-doc-figs target
# modified to install the figures with html manuals
install-html-figs:
	if test "x$(htmldir)" = "x$(docdir)"; then \
		_instdir="$(DESTDIR)$(pkgdocdir)/html"; \
	else \
		_instdir="$(DESTDIR)$(htmldir)"; \
	fi; \
	list='$(patsubst C/%,%,$(_DOC_C_FIGURES))'; \
	for lc in C $(_DOC_REAL_LINGUAS); do \
	  if test -f "$(srcdir)/$$lc/$(DOC_MODULE).html"; then \
		for fig in $$list; do \
			figsymlink=false; \
			if test -f "$(srcdir)/$$lc/$$fig" -a ! -h "$(srcdir)/$$lc/$$fig"; then \
				figfile="$(srcdir)/$$lc/$$fig"; \
			else \
				figsymlink=true; \
			fi; \
			figdir=`echo $$fig | sed -e 's/^\(.*\/\).*/\1/' -e '/\//!s/.*//'`; \
			figdir="$$_instdir/$$lc/nact/$$figdir"; \
			if ! test -d "$$figdir"; then cmd="$(mkinstalldirs) $$figdir"; echo $$cmd; (eval $$cmd); fi; \
			if $$figsymlink; then \
				cmd="cd $$figdir && $(LN_S) -f ../../../C/nact/$$fig"; \
			else \
				cmd="$(INSTALL_DATA) $$figfile $$figdir/"; \
			fi; \
			echo $$cmd; (eval $$cmd); \
		done; \
	  fi; \
	done

# A workaround against the bug #636175 (do not install admon-* icons)
# Install admon icons as a admon/ subdirectory for C language
# other languages are symlinked to this one
install-html-admon:
	admon_dir=/usr/share/gnome-doc-utils/icons/hicolor/48x48/status; \
	if test -d $$admon_dir; then \
		admon_src=$$admon_dir; \
	else \
		admon_src=$(srcdir)/C/admon; \
	fi; \
	echo "admon_src=$$admon_src"; \
	if test -d "$$admon_src"; then \
		if test "x$(htmldir)" = "x$(docdir)"; then \
			_instdir="$(DESTDIR)$(pkgdocdir)/html"; \
		else \
			_instdir="$(DESTDIR)$(htmldir)"; \
		fi; \
		if test -f "$(srcdir)/C/$(DOC_MODULE).html"; then \
			cmd="$(mkinstalldirs) \"$$_instdir/C/nact/admon\""; echo $$cmd; (eval $$cmd); \
			cmd="cp -v $$admon_src/*.png \"$$_instdir/C/nact/admon/\""; echo $$cmd; (eval $$cmd); \
			for lc in $(_DOC_REAL_LINGUAS); do \
				if test -f "$(srcdir)/$$lc/$(DOC_MODULE).html"; then \
					cmd="cd \"$$_instdir/$$lc/nact\"; $(LN_S) -f ../../C/nact/admon"; echo $$cmd; (eval $$cmd); \
				fi; \
			done; \
		fi; \
	fi

uninstall-html-manuals:
	if test "x$(htmldir)" = "x$(docdir)"; then \
		_instdir="$(DESTDIR)$(pkgdocdir)/html"; \
	else \
		_instdir="$(DESTDIR)$(htmldir)"; \
	fi; \
	find $$_instdir -type d -name 'admon' | xargs rm -vfr; \
	find $$_instdir -type l -name 'admon' | xargs rm -vf; \
	find $$_instdir -type d -name 'figures' | xargs rm -vfr; \
	find $$_instdir -type d -name 'stylesheet-images' | xargs rm -vfr; \
	find $$_instdir -type f -name '*.html' | xargs rm -vf; \
	find $$_instdir -type d | sort -r | xargs rmdir -v

# locale figures (_DOC_LC_FIGURES) are distributed by gnome-doc-utils
# when they exist
# - distribute admon-* icons as a C/ subdirectory
# - distribute *.html manuals
# This obviously suppose that gnome-doc-utils package is installed
dist-html-manuals-hook: all-html-manuals dist-html-admon dist-html-doc

dist-html-admon:
	admon_dir=/usr/share/gnome-doc-utils/icons/hicolor/48x48/status; \
	if test -d $$admon_dir; then \
		if test -f $(srcdir)/C/$(DOC_MODULE).html; then \
			cmd="$(mkinstalldirs) \"$(distdir)/C/admon\""; echo $$cmd; (eval $$cmd); \
			cmd="cp -vp $$admon_dir/*.png $(distdir)/C/admon/"; echo $$cmd; (eval $$cmd); \
		fi; \
	fi

dist-html-doc:
	for lc in C $(_DOC_REAL_LINGUAS); do \
		if test -f "$(srcdir)/$$lc/$(DOC_MODULE).html"; then \
			cmd="cp $(srcdir)/$$lc/$(DOC_MODULE).html $(distdir)/$$lc/"; echo $$cmd; (eval $$cmd); \
			if test -d "$(srcdir)/$$lc/stylesheet-images"; then \
				cmd="cp -vrp \"$(srcdir)/$$lc/stylesheet-images\" $(distdir)/$$lc/"; echo $$cmd; (eval $$cmd); \
			fi; \
		fi; \
	done

# Build pdf manuals as $(srcdir)/<lang>/<module>.pdf
#
# The rule here is: only rebuild .pdf files if corresponding xml has changed
# .xml themselves are distributed, so are not modified when compiling from a
# distribution
#
# We only use dblatex for now

MANUALS_PDF = $(MANUALS_XML:%.xml=%.pdf)

MAINTAINERCLEANFILES += $(MANUALS_PDF)

if ENABLE_PDF_MANUALS
all-pdf-manuals: $(MANUALS_PDF)
else
all-pdf-manuals:
endif

# _DOC_LC_DOCS is the list of the translated (not C) DOC_MODULE.xml files
# we so are sure that they exist and are up to date
# (path is .po ->[msgfmt]-> .mo ->[xml2po]-> .xml)
$(MANUALS_PDF): %.pdf: %.xml
	@lc=$<; \
	lc=$${lc%/*}; \
	echo "Generating $@ ..."; \
	if ! test -d "$$lc/figures"; then cmd="$(MKDIR_P) $$lc/figures"; (eval $$cmd); fi; \
	list='$(patsubst C/%,%,$(_DOC_C_FIGURES))'; \
	for fig in $$list; do \
		if ! test -f "$$lc/$$fig"; then \
			cmd="(cd $$lc/figures && $(LN_S) -f ../../$(srcdir)/C/$$fig)"; (eval $$cmd); \
		fi; \
	done; \
	cmd="dblatex --output $@ $<"; echo $$cmd; (eval $$cmd); \
	find $$lc/figures -type l | xargs rm -f; \
	rmdir $$lc/figures 2>/dev/null || :

# if --pdfdir has not been specified, then we default to PKGDOCDIR/pdf
# in all case, we append the locale as a subdirectory level
#
# as pdf are monobloc documents, we do not create a per-document subdir

if ENABLE_PDF_MANUALS
install-pdf-manuals: all-pdf-manuals
	if test "$(pdfdir)" = "$(docdir)"; then \
		_instdir=$(DESTDIR)$(pkgdocdir)/pdf; \
	else \
		_instdir=$(DESTDIR)$(pdfdir); \
	fi; \
	for lc in C $(_DOC_REAL_LINGUAS); do \
		if test -f $(srcdir)/$$lc/$(DOC_MODULE).pdf; then \
			if ! test -d $$_instdir/$$lc; then cmd="$(MKDIR_P) $$_instdir/$$lc"; echo $$cmd; (eval $$cmd); fi; \
			cmd="$(INSTALL_DATA) \"$(srcdir)/$$lc/$(DOC_MODULE).pdf\" \"$$_instdir/$$lc/\""; echo $$cmd; (eval $$cmd); \
		fi; \
	done
else
install-pdf-manuals:
endif

uninstall-pdf-manuals:
	if test "$(pdfdir)" = "$(docdir)"; then \
		_instdir=$(DESTDIR)$(pkgdocdir)/pdf; \
	else \
		_instdir=$(DESTDIR)$(pdfdir); \
	fi; \
	find $$_instdir -name '*.pdf' | xargs rm -vf; \
	find $$_instdir -type d | sort -r | xargs rmdir -v

dist-pdf-manuals-hook:
	for lc in C $(_DOC_REAL_LINGUAS); do \
		if test -f "$(srcdir)/$$lc/$(DOC_MODULE).pdf"; then \
			cmd="cp $(srcdir)/$$lc/$(DOC_MODULE).pdf $(distdir)/$$lc/"; echo $$cmd; (eval $$cmd); \
		fi; \
	done

# According to GNU Make manual (§ 13 What Gets Cleaned):
#
# - mostlyclean deletes files we typically want to rebuild
# - clean deletes other files built with make
# - distclean deletes files built by configure (none here)
##
##  none of these target should delete any distributed file
##
# - maintainer-clean should delete other files
clean-local:
	rm -f *~
	rm -f *.stamp

distclean-local: distclean-html distclean-pdf

distclean-html:
	find $(srcdir) -type l -name 'admon' | xargs rm -vf; \
	for d in `find $(srcdir) -type d -name 'figures'`; do find $$d -type l | xargs rm -vf; done

distclean-pdf:

maintainer-clean-local: maintainer-clean-html maintainer-clean-pdf

maintainer-clean-html:
	find $(srcdir) -type f -name 'admon/*.png' | xargs rm -vf; \
	rmdir -v $(srcdir)/C/admon; \
	find $(srcdir) -type d -name 'stylesheet-images' | xargs rm -vfr

maintainer-clean-pdf:

endif
