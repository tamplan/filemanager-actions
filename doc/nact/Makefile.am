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
# files in doc/nact/<lang>/. When installing, g-d-t installs these help
# files in <datadir>/gnome/help/nautilus-actions-config-tool/<lang>.
# At install time, missing tranlated images are replace with symlinks to
# ../../C/figures/<image.png>.

include $(top_srcdir)/gnome-doc-utils.make

DOC_MODULE = nautilus-actions-config-tool

DOC_ENTITIES = \
	appendix-gnu-fdl-1.3.xml						\
	articleinfo.xml									\
	legalnotice.xml									\
	nact-bugs.xml									\
	nact-conditions.xml								\
	nact-edit.xml									\
	nact-execution.xml								\
	nact-export.xml									\
	nact-import.xml									\
	nact-menus.xml									\
	nact-misc.xml									\
	nact-prefs.xml									\
	nact-profiles.xml								\
	nact-save.xml									\
	nact-sorting.xml								\
	nact-starting.xml								\
	nact-where.xml

DOC_INCLUDES =

DOC_FIGURES = \
	figures/add.png									\
	figures/nact-action-tab.png						\
	figures/nact-basenames-tab.png					\
	figures/nact-capabilities-tab.png				\
	figures/nact-command-tab.png					\
	figures/nact-desktop-environment-frame.png		\
	figures/nact-environment-tab.png				\
	figures/nact-exec-mode-frame.png				\
	figures/nact-execute-as-frame.png				\
	figures/nact-execution-environment-frame.png	\
	figures/nact-execution-tab.png					\
	figures/nact-folders-tab.png					\
	figures/nact-legend.png							\
	figures/nact-main-empty.png						\
	figures/nact-mimetypes-tab.png					\
	figures/nact-new-capability.png					\
	figures/nact-new-scheme.png						\
	figures/nact-preferences-export.png				\
	figures/nact-preferences-import.png				\
	figures/nact-preferences-io-providers.png		\
	figures/nact-preferences-runtime.png			\
	figures/nact-preferences-schemes.png			\
	figures/nact-preferences-ui.png					\
	figures/nact-properties-tab.png					\
	figures/nact-schemes-tab.png					\
	figures/nact-selection-count-frame.png			\
	figures/nact-startup-mode-frame.png				\
	figures/nact-target-location.png				\
	figures/nact-working-directory-frame.png		\
	figures/read-only.png							\
	figures/remove.png								\
	figures/sort-asc.png							\
	figures/sort-buttons-frame.png					\
	figures/sort-desc.png							\
	figures/sort-manual.png							\
	figures/start-context-menu.png					\
	figures/start-context-menu-nautilus.png			\
	figures/start-zenity-result.png

DOC_LINGUAS = de es

pkgdocdir = $(datarootdir)/doc/@PACKAGE@-@VERSION@

all-local: all-manuals

all-manuals: all-html-manuals all-pdf-manuals

install-data-local: install-manuals

install-manuals: install-html-manuals install-pdf-manuals

uninstall-local: uninstall-manuals

uninstall-manuals: uninstall-html-manuals uninstall-pdf-manuals

dist-hook: doc-dist-hook dist-html-manuals-hook dist-pdf-manuals-hook

# Build HTML manuals as <lang>/html/<module>.html
#
# The '_DOC_LC_DOCS' prerequisite ensures that the translated .xml doc
# has been generated (path is po -> mo -> xml)
#
# An alternative for generating HTML manual is
# gnome-doc-tool html -o xhtml -d0 nautilus-actions-config-tool.xml
# which is part of gnome-doc-utils package:
# - xhtml subdirectory has to be previously created
# - figures subdirectory has to be copied to xhtml/
# - there is (as of 2010-09-03) no icon for notes and tips.
# The produced output is very close of what Yelp gives, with in particular
# summaries in each chapter while db2html does not produce them
#
# Contrarily, gnome-doc-tool does not produce the legal and abstract
# in top of the document, while db2html does.
#
# Both db2html and gnome-doc-tool do not honor external links;
# they are converted to simple links which target the same frame window

if ENABLE_HTML_MANUALS
all-html-manuals: html.stamp
	@echo "in all-html-manuals"

html.stamp: $(_DOC_LC_DOCS)
	@echo "in html-stamp"
	@-for i in C $(_DOC_REAL_LINGUAS); do \
		rm -fr $$i/$(DOC_MODULE).junk; \
		if ! test -d $$i; then $(MKDIR_P) $$i; fi; \
		(cd $$i; \
		 if test "x$(WITH_DB2HTML)" = "xyes"; then \
			echo "Generating $$i/html/$(DOC_MODULE).html ..."; \
			db2html --nochunks $(DOC_MODULE).xml; \
			mv -v $(DOC_MODULE) html; \
		 fi; \
		 if test "x$(WITH_GDT)" = "xyes"; then \
			echo "Generating $$i/html/$(DOC_MODULE).html ..."; \
		 	if ! test -d html; then $(MKDIR_P) html; fi; \
			gnome-doc-tool html -o html -d0 $(DOC_MODULE).xml; \
		 fi; \
		 chmod -R u+w html; \
		); \
	done
	touch html.stamp
else
all-html-manuals:
endif

# Install HTML manuals if they exist (do not try to rebuild them)
#
# If --htmldir has not been specified, then we default to PKGDOCDIR/html
# in all case, we append the locale as a subdirectory level
#
# Note that we do not honor here the --with-html-dir gtk-doc option,
# but the --htmldir autotools one
#
# As html have multiple files, we create a per-document subdirectory
# as <pkgdocdir>/html/<lc>/nact/<module>.html

install-html-manuals: install-html-figs
	@echo "in install-html-manuals"
	@-for _lc in C $(_DOC_REAL_LINGUAS); do \
		if test "$(htmldir)" = "$(docdir)"; then \
			_destdir=$(DESTDIR)$(pkgdocdir)/html/$$_lc/nact; \
		else \
			_destdir=$(DESTDIR)$(htmldir)/$$_lc/nact; \
		fi; \
		if ! test -d $$_destdir; then $(MKDIR_P) $$_destdir; fi; \
		_srcdir=$(top_builddir)/doc/nact/$$_lc/html; \
		find $$_srcdir -type f | grep -v '/figures/' | while read _f; do \
			_dname=`dirname $$_f`; \
			_subdir=`echo "$$_dname" | sed "s|^$$_srcdir[/]*||"`; \
			if ! test -d "$$_destdir/$$_subdir"; then $(MKDIR_P) "$$_destdir/$$_subdir"; fi; \
			_bname=`basename "$$_f"`; \
			echo $(INSTALL_DATA) "$$_f" "$$_destdir/$$_subdir/$$_bname"; \
			$(INSTALL_DATA) "$$_f" "$$_destdir/$$_subdir/$$_bname"; \
		done; \
	done

uninstall-html-manuals:
	if test "$(htmldir)" = "$(docdir)"; then \
		_instdir=$(DESTDIR)$(pkgdocdir)/html; \
	else \
		_instdir=$(DESTDIR)$(htmldir); \
	fi; \
	rm -fr $$_instdir

# copied from gnome-doc-utils.make install-doc-figs target
# modified to install the figures with html manuals
install-html-figs:
	@echo "in install-html-figs"
	if test "$(htmldir)" = "$(docdir)"; then \
		_instdir=$(DESTDIR)$(pkgdocdir)/html; \
	else \
		_instdir=$(DESTDIR)$(htmldir); \
	fi; \
	list='$(patsubst C/%,%,$(_DOC_C_FIGURES))'; \
	for lc in C $(_DOC_REAL_LINGUAS); do \
	  if test -f $(top_builddir)/doc/nact/$$lc/html/$(DOC_MODULE).html; then \
		for fig in $$list; do \
			figsymlink=false; \
			if test -f "$$lc/$$fig"; then \
				figfile="$$lc/$$fig"; \
			elif test -f "$(srcdir)/$$lc/$$fig"; then \
				figfile="$(srcdir)/$$lc/$$fig"; \
			else \
				figsymlink=true; \
			fi; \
			figdir=`echo $$fig | sed -e 's/^\(.*\/\).*/\1/' -e '/\//!s/.*//'`; \
			figdir="$$_instdir/$$lc/nact/$$figdir"; \
			if ! test -d "$$figdir"; then echo "$(mkinstalldirs) $$figdir"; $(mkinstalldirs) "$$figdir"; fi; \
			figbase=`echo $$fig | sed -e 's/^.*\///'`; \
			if $$figsymlink; then \
				echo "cd $$figdir && $(LN_S) -f ../../C/$$fig $$figbase"; \
				( cd "$$figdir" && $(LN_S) -f "../../C/$$fig" "$$figbase" ); \
			else \
				echo "$(INSTALL_DATA) $$figfile $$figdir$$figbase"; \
				$(INSTALL_DATA) "$$figfile" "$$figdir$$figbase"; \
			fi; \
		done; \
	  fi; \
	done

dist-html-manuals-hook:
	list='$(patsubst C/%,%,$(_DOC_C_FIGURES))'; \
	for lc in C $(_DOC_REAL_LINGUAS); do \
	  if test -f $(top_builddir)/doc/nact/$$lc/html/$(DOC_MODULE).html; then \
		$(MKDIR_P) $(distdir)/$$lc/html; \
		cp $(top_builddir)/doc/nact/$$lc/html/* $(distdir)/$$lc/html/; \
		for fig in $$list; do \
			figsymlink=false; \
			if test -f "$$lc/$$fig"; then \
				figfile="$$lc/$$fig"; \
			elif test -f "$(srcdir)/$$lc/$$fig"; then \
				figfile="$(srcdir)/$$lc/$$fig"; \
			else \
				figsymlink=true; \
			fi; \
			figdir=`echo $$fig | sed -e 's/^\(.*\/\).*/\1/' -e '/\//!s/.*//'`; \
			figdir="$$distdir/$$lc/html/$$figdir"; \
			if ! test -d "$$figdir"; then echo "$(mkinstalldirs) $$figdir"; $(mkinstalldirs) "$$figdir"; fi; \
			figbase=`echo $$fig | sed -e 's/^.*\///'`; \
			if $$figsymlink; then \
				echo "cd $$figdir && $(LN_S) -f ../../C/$$fig $$figbase"; \
				( cd "$$figdir" && $(LN_S) -f "../../C/$$fig" "$$figbase" ); \
			else \
				echo "$(INSTALL_DATA) $$figfile $$figdir$$figbase"; \
				$(INSTALL_DATA) "$$figfile" "$$figdir$$figbase"; \
			fi; \
		done; \
	  fi; \
	done
	find $(distdir) -type d -exec chmod 755 {} \;
	find $(distdir) -type f -exec chmod 644 {} \;

# Build pdf manuals as <lang>/<module>.pdf
if ENABLE_PDF_MANUALS
all-pdf-manuals: pdf-figs.stamp pdf.stamp
	@echo "in all-pdf-manuals"

pdf.stamp: $(_DOC_LC_DOCS)
	@echo "in pdf.stamp"
	@-for i in C $(_DOC_REAL_LINGUAS); do \
		_target=$$i/$(DOC_MODULE).pdf; \
		echo "Generating $$_target ..."; \
		if ! test -d $$i; then $(MKDIR_P) $$i; fi; \
		echo "dblatex --output $$_target $(top_srcdir)/doc/nact/$$i/$(DOC_MODULE).xml"; \
		dblatex --output $$_target $(top_srcdir)/doc/nact/$$i/$(DOC_MODULE).xml; \
	done
	touch pdf.stamp

# copied from gnome-doc-utils.make install-doc-figs target
# modified to have the figures available when building pdf manuals
# not trying to symlinl C/figures/ 
pdf-figs.stamp:
	@echo "in build-pdf-figs"
	@list='$(patsubst C/%,%,$(_DOC_C_FIGURES))'; for fig in $$list; do \
	  for lc in $(_DOC_REAL_LINGUAS); do \
	    figdir="$$lc/"`echo $$fig | sed -e 's/^\(.*\/\).*/\1/' -e '/\//!s/.*//'`; \
	    figdir="$(builddir)/$$figdir"; \
	    if ! test -d "$$figdir"; then echo "$(MKDIR_P) $$figdir"; $(MKDIR_P) "$$figdir"; fi; \
	    figbase=`echo $$fig | sed -e 's/^.*\///'`; \
	    if ! test -f "$$lc/$$fig"; then \
	      echo "cd $$figdir && $(LN_S) -f ../../C/$$fig $$figbase"; \
	      ( cd "$$figdir" && $(LN_S) -f "../../C/$$fig" "$$figbase" ); \
	    fi; \
	  done; \
	done
	touch pdf-figs.stamp
else
all-pdf-manuals:
endif

# if --pdfdir has not been specified, then we default to PKGDOCDIR/pdf
# in all case, we append the locale as a subdirectory level
#
# as pdf are monobloc documents, we do not create a per-document subdir
install-pdf-manuals:
	@echo "in install-pdf-manuals"
	@-for _lc in C $(_DOC_REAL_LINGUAS); do \
		_srcdir=$(top_builddir)/doc/nact/$$_lc; \
		if test "$(pdfdir)" = "$(docdir)"; then \
			_destdir=$(DESTDIR)$(pkgdocdir)/pdf/$$_lc; \
		else \
			_destdir=$(DESTDIR)$(pdfdir)/$$_lc; \
		fi; \
		if ! test -d $$_destdir; then $(MKDIR_P) $$_destdir; fi; \
		echo $(INSTALL_DATA) "$$_srcdir/$(DOC_MODULE).pdf" "$$_destdir/$(DOC_MODULE).pdf"; \
		$(INSTALL_DATA) "$$_srcdir/$(DOC_MODULE).pdf" "$$_destdir/$(DOC_MODULE).pdf"; \
	done

uninstall-pdf-manuals:
	if test "$(pdfdir)" = "$(docdir)"; then \
		_instdir=$(DESTDIR)$(pkgdocdir)/pdf; \
	else \
		_instdir=$(DESTDIR)$(pdfdir); \
	fi; \
	rm -fr $$_instdir

dist-pdf-manuals-hook:
	for _lc in C $(_DOC_REAL_LINGUAS); do \
		$(MKDIR_P) $(distdir)/pdf/$$_lc; \
		cp $(top_builddir)/doc/nact/$$_lc/$(DOC_MODULE).pdf $(distdir)/$$_lc/; \
	done
	find $(distdir) -type d -exec chmod 755 {} \;
	find $(distdir) -type f -exec chmod 644 {} \;

clean-local: clean-html-manuals clean-pdf-manuals clean-figs
	rm -f *~
	rm -f *.stamp

distclean-local: clean-html-manuals clean-pdf-manuals

clean-html-manuals:
	for lc in C $(_DOC_REAL_LINGUAS); do \
		rm -fr $$lc/$(DOC_MODULE); \
		rm -fr $$lc/$(DOC_MODULE).junk; \
		rm -fr $$lc/html; \
	done

clean-pdf-manuals:
	for lc in C $(_DOC_REAL_LINGUAS); do \
		rm -f $$lc/*.pdf; \
	done

clean-figs:
	for lc in $(_DOC_REAL_LINGUAS); do \
		if test -d $$lc/figures; then \
			for l in `find $$lc/figures -type l`; do rm -vf $$l; done; \
			(rmdir $$lc/figures 2>/dev/null && echo "rmdir $$lc/figures"); \
		fi; \
	done
