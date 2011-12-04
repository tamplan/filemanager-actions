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
#
# Note 1. Do not use the $(NULL) syntax here as this may prevent Damned-Lies
# to correctly generate POT files (cf. mail of Claude Paroz 2010-08-29
# http://mail.gnome.org/archives/gnome-i18n/2010-August/msg00229.html).

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

# set to '-v' to be verbose (much verbose!)
opt_verbose =

all-local: all-manuals all-html-manuals all-pdf-manuals

.PHONY: all-manuals
all-manuals: all-html-manuals all-pdf-manuals

install-data-local: install-manuals

.PHONY: install-manuals
install-manuals: install-html-manuals install-pdf-manuals

uninstall-local: uninstall-manuals

.PHONY: uninstall-manuals
uninstall-manuals: uninstall-html-manuals uninstall-pdf-manuals

# doc-dist-hook is the dist hook from gnome-doc-utils.make
# it distributes both C (original) and generated (translated) .xml files,
# but not generated .omf files which so will have to be re-generated from
# the distributed .omf.in file
dist-hook: doc-dist-hook dist-manuals-hook

.PHONY: dist-manuals-hook
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

.PHONY: all-html-manuals

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
# distribution (though this is not the intended use).
#
# pwd=/home/pierre/data/eclipse/nautilus-actions/nautilus-actions-3.1.5/_build/docs/nact
# target=C/nautilus-actions-config-tool.html
# source=/home/pierre/net/eclipse/nautilus-actions/docs/nact/C/nautilus-actions-config-tool.xml
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
	@if test "$(opt_verbose)" = ""; then echo "  HTML   $@"; fi; \
	( \
	 dirxml=$<; dirxml=$${dirxml%/*}; cd $$dirxml; \
	 fxml=$<; fxml=$${fxml##*/}; \
	 if test "x$(WITH_DB2HTML)" = "xyes"; then \
		cmd="db2html --output . --nochunks $$fxml"; \
		if test "$(opt_verbose)" != ""; then echo "$$cmd"; fi; \
		eval $$cmd || :; \
		rm -fr stylesheet-images; \
	    mv $(opt_verbose) $(DOC_MODULE)/stylesheet-images .; rmdir $(opt_verbose) $(DOC_MODULE); \
	    find . -type d -name '*.junk' | xargs rm -fr; \
	    find . -type d -name 'docbook2html-dir' | xargs rm -fr; \
	 fi; \
	 if test "x$(WITH_GDT)" = "xyes"; then \
		 fhtml=$@; fhtml=$${fhtml##*/}; \
		 cmd="gnome-doc-tool html -o . -d0 $$fxml"; \
		 if test "$(opt_verbose)" != ""; then echo "$$cmd"; fi; \
		 eval $$cmd; \
		 if test -f $$fhtml; then \
			 sed -i -e 's?url("admon?url("admon/admon?' $$fhtml; \
			 admon_dir=/usr/share/gnome-doc-utils/icons/hicolor/48x48/status; \
			 if test -d $$admon_dir; then \
				lc=$${dirxml##*/}; \
				if test "$$lc" = "C"; then \
					cmd="$(mkinstalldirs) admon"; eval $$cmd; \
					cmd="cp $(opt_verbose) $$admon_dir/*.png admon/"; eval $$cmd; \
				else \
					cmd="$(LN_S) -f ../C/admon)"; eval $$cmd; \
					if test "$(opt_verbose)" != ""; then echo $$cmd; fi; \
				fi; \
			fi; \
		fi; \
	 fi; \
	)

# Install HTML manuals
# in <htmldir>/<lang>/nact/ subdirectory; htmldir defaulting
# to <pkgdocdir>/html
#
# Note: we do honor here --htmldir autotools configuration option.
#
# Note: workaround against the bug #636175 (do not install admon-* icons)
# Install admon icons as a admon/ subdirectory for C language
# other languages are symlinked to this one

.PHONY: install-html-manuals
install-html-manuals: install-html-figs
	@for lc in C $(_DOC_REAL_LINGUAS); do \
		if test "x$(htmldir)" = "x$(docdir)"; then \
			_instdir="$(DESTDIR)$(pkgdocdir)/html/$$lc/nact"; \
		else \
			_instdir="$(DESTDIR)$(htmldir)/$$lc/nact"; \
		fi; \
		if test -f "$(srcdir)/$$lc/$(DOC_MODULE).html"; then d=$(srcdir); fi; \
		if test -f "$(builddir)/$$lc/$(DOC_MODULE).html"; then d=$(builddir); fi; \
		if test -f "$$d/$$lc/$(DOC_MODULE).html"; then \
			if test "$(opt_verbose)" = ""; then echo "  Installing $$_instdir/$(DOC_MODULE).html ..."; fi; \
			cp -p $(opt_verbose) "$$d/$$lc/$(DOC_MODULE).html" $$_instdir/; \
			subdir="stylesheet-images"; \
			if test -d "$$d/$$lc/$$subdir"; then \
				if test "$(opt_verbose)" = ""; then echo "  Installing $$_instdir/$$subdir ..."; fi; \
				cmd="cp -rp $(opt_verbose) \"$$d/$$lc/$$subdir\" $$_instdir/"; eval $$cmd; \
				if test "$(opt_verbose)" != ""; then echo $$cmd; fi; \
			fi; \
			subdir="admon"; \
			if test -d "$$d/$$lc/$$subdir"; then \
				if test "$(opt_verbose)" = ""; then echo "  Installing $$_instdir/$$subdir ..."; fi; \
				cmd="cp -rp $(opt_verbose) \"$$d/$$lc/$$subdir\" $$_instdir/"; eval $$cmd; \
				if test "$(opt_verbose)" != ""; then echo $$cmd; fi; \
			fi; \
			if test -h "$$d/$$lc/$$subdir"; then \
				if test "$(opt_verbose)" = ""; then echo "  Installing $$_instdir/$$subdir ..."; fi; \
				cmd="(cd \"$$_instdir/$$lc/nact\"; $(LN_S) -f ../../C/nact/$$subdir)"; eval $$cmd; \
				if test "$(opt_verbose)" != ""; then echo $$cmd; fi; \
			fi; \
		fi; \
	done

# copied from gnome-doc-utils.make install-doc-figs target
# modified to install the figures with html manuals
.PHONY: install-html-figs
install-html-figs:
	@if test "x$(htmldir)" = "x$(docdir)"; then \
		_instdir="$(DESTDIR)$(pkgdocdir)/html"; \
	else \
		_instdir="$(DESTDIR)$(htmldir)"; \
	fi; \
	list='$(patsubst C/%,%,$(_DOC_C_FIGURES))'; \
	for lc in C $(_DOC_REAL_LINGUAS); do \
	  if test "$(opt_verbose)" = ""; then echo "  Installing $$_instdir/$$lc/nact/figures ..."; fi; \
	  if test -f "$(srcdir)/$$lc/$(DOC_MODULE).html"; then d=$(srcdir); fi; \
	  if test -f "$(builddir)/$$lc/$(DOC_MODULE).html"; then d=$(builddir); fi; \
	  if test -f "$$d/$$lc/$(DOC_MODULE).html"; then \
		for fig in $$list; do \
			figsymlink=false; \
			if test -f "$(srcdir)/$$lc/$$fig" -a ! -h "$(srcdir)/$$lc/$$fig"; then \
				figfile="$(srcdir)/$$lc/$$fig"; \
			else \
				figsymlink=true; \
			fi; \
			figdir="$$_instdir/$$lc/nact/$${fig%/*}"; \
			if ! test -d "$$figdir"; then cmd="$(MKDIR_P) $$figdir"; eval $$cmd; fi; \
			if $$figsymlink; then \
				cmd="(cd $$figdir && $(LN_S) -f ../../../C/nact/$$fig)"; \
			else \
				cmd="$(INSTALL_DATA) $$figfile $$figdir/"; \
			fi; \
			if test "$(opt_verbose)" != ""; then echo $$cmd; fi; \
			eval $$cmd; \
		done; \
	  fi; \
	done

.PHONY: uninstall-html-manuals
uninstall-html-manuals:
	@if test "x$(htmldir)" = "x$(docdir)"; then \
		_instdir="$(DESTDIR)$(pkgdocdir)/html"; \
	else \
		_instdir="$(DESTDIR)$(htmldir)"; \
	fi; \
	find $$_instdir -type d -name 'admon' | xargs rm -fr $(opt_verbose); \
	find $$_instdir -type l -name 'admon' | xargs rm -f $(opt_verbose); \
	find $$_instdir -type d -name 'figures' | xargs rm -fr $(opt_verbose); \
	find $$_instdir -type d -name 'stylesheet-images' | xargs rm -fr $(opt_verbose); \
	find $$_instdir -type f -name '*.html' | xargs rm -f $(opt_verbose); \
	find $$_instdir -type d | sort -r | xargs rmdir $(opt_verbose)

# locale figures (_DOC_LC_FIGURES) are distributed by gnome-doc-utils
# when they exist
# - distribute admon-* icons as a C/ subdirectory
# - distribute *.html manuals

.PHONY: dist-html-manuals-hook
dist-html-manuals-hook:
	@for lc in C $(_DOC_REAL_LINGUAS); do \
		if test -f "$(srcdir)/$$lc/$(DOC_MODULE).html"; then d=$(srcdir); fi; \
		if test -f "$(builddir)/$$lc/$(DOC_MODULE).html"; then d=$(builddir); fi; \
		if test -f "$$d/$$lc/$(DOC_MODULE).html"; then \
			cmd="cp $(opt_verbose) $$d/$$lc/$(DOC_MODULE).html $(distdir)/$$lc/"; eval $$cmd; \
			if test -d "$$d/$$lc/stylesheet-images"; then \
				echo "distributing stylesheet-images"; \
				cmd="cp -rp $(opt_verbose) \"$$d/$$lc/stylesheet-images\" $(distdir)/$$lc/"; eval $$cmd; \
			fi; \
			if test -d "$$d/$$lc/admon" -o -h "$$d/$$lc/admon"; then \
				echo "distributing admon"; \
				cmd="cp -rp $(opt_verbose) \"$$d/$$lc/admon\" $(distdir)/$$lc/"; eval $$cmd; \
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

.PHONY: all-pdf-manuals

if ENABLE_PDF_MANUALS
all-pdf-manuals: $(MANUALS_PDF)
else
all-pdf-manuals:
endif

# _DOC_LC_DOCS is the list of the translated (not C) DOC_MODULE.xml files
# we so are sure that they exist and are up to date
# (path is .po ->[msgfmt]-> .mo ->[xml2po]-> .xml)
$(MANUALS_PDF): %.pdf: %.xml
	@if test "$(opt_verbose)" = ""; then echo "  PDF    $@"; fi; \
	( \
	 dirxml=$<; dirxml=$${dirxml%/*}; cd $$dirxml; \
	 fxml=$<; fxml=$${fxml##*/}; \
	 lc=$@; \
	 lc=$${lc%/*}; \
	 $(MKDIR_P) figures; \
	 list='$(patsubst C/%,%,$(_DOC_C_FIGURES))'; \
	 for fig in $$list; do \
		if test -f "$(srcdir)/$$lc/$$fig"; then d=$$lc; else d="C"; fi; \
		if ! test -f $$fig; then \
			cmd="(cd figures && $(LN_S) -f $(srcdir)/$$d/$$fig)"; eval $$cmd; \
		fi; \
	 done; \
	 cmd="dblatex $$fxml"; eval $$cmd; \
	 find figures -type l | xargs rm -f; \
	 rmdir figures 2>/dev/null || :; \
	)

# if --pdfdir has not been specified, then we default to PKGDOCDIR/pdf
# in all case, we append the locale as a subdirectory level
#
# as pdf are monobloc documents, we do not create a per-document subdir

.PHONY: install-pdf-manuals
install-pdf-manuals:
	@if test "$(pdfdir)" = "$(docdir)"; then \
		_instdir=$(DESTDIR)$(pkgdocdir)/pdf; \
	else \
		_instdir=$(DESTDIR)$(pdfdir); \
	fi; \
	for lc in C $(_DOC_REAL_LINGUAS); do \
		if test -f $(srcdir)/$$lc/$(DOC_MODULE).pdf; then d=$(srcdir); fi; \
		if test -f $(builddir)/$$lc/$(DOC_MODULE).pdf; then d=$(builddir); fi; \
		if test -f $$d/$$lc/$(DOC_MODULE).pdf; then \
			if test "$(opt_verbose)" = ""; then echo "  Installing $$_instdir/$$lc/$(DOC_MODULE).pdf ..."; fi; \
			if ! test -d $$_instdir/$$lc; then cmd="$(MKDIR_P) $$_instdir/$$lc"; eval $$cmd; fi; \
			cmd="$(INSTALL_DATA) \"$$d/$$lc/$(DOC_MODULE).pdf\" \"$$_instdir/$$lc/\""; eval $$cmd; \
			if test "$(opt_verbose)" != ""; then echo $$cmd; fi; \
		fi; \
	done

.PHONY: uninstall-pdf-manuals
uninstall-pdf-manuals:
	@if test "$(pdfdir)" = "$(docdir)"; then \
		_instdir=$(DESTDIR)$(pkgdocdir)/pdf; \
	else \
		_instdir=$(DESTDIR)$(pdfdir); \
	fi; \
	find $$_instdir -name '*.pdf' | xargs rm -f $(opt_verbose); \
	find $$_instdir -type d | sort -r | xargs rmdir $(opt_verbose)

.PHONY: dist-pdf-manuals-hook
dist-pdf-manuals-hook:
	@for lc in C $(_DOC_REAL_LINGUAS); do \
		if test -f $(srcdir)/$$lc/$(DOC_MODULE).pdf; then d=$(srcdir); fi; \
		if test -f $(builddir)/$$lc/$(DOC_MODULE).pdf; then d=$(builddir); fi; \
		if test -f "$$d/$$lc/$(DOC_MODULE).pdf"; then \
			cmd="cp $$d/$$lc/$(DOC_MODULE).pdf $(distdir)/$$lc/"; eval $$cmd; \
			if test "$(opt_verbose)" != ""; then echo $$cmd; fi; \
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

distclean-pdf:

maintainer-clean-local: maintainer-clean-html maintainer-clean-pdf

maintainer-clean-html:
	find $(srcdir) -type d -name 'admon' | xargs rm -fr
	find $(builddir) -type l -name 'admon' | xargs rm -f
	find $(srcdir) -type d -name 'stylesheet-images' | xargs rm -fr
	find $(builddir) -type d -name 'stylesheet-images' | xargs rm -fr

maintainer-clean-pdf:

# we have to override gnome-doc-utils.make:clean-doc-dir recipe
# as it tries to remove directories which are not empty as we have
# not yet deleted the files we putted on
clean-doc-dir:
	@:

endif
