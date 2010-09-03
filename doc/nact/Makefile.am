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
# to correctly generate POT files (cf. mail of Claude Paroz 2008-08-29)

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
	figures/remove.png								\
	figures/sort-asc.png							\
	figures/sort-buttons-frame.png					\
	figures/sort-desc.png							\
	figures/sort-manual.png							\
	figures/start-context-menu.png					\
	figures/start-context-menu-nautilus.png			\
	figures/start-zenity-result.png

DOC_LINGUAS = de es

# generate HTML and PDF NACT user's manual if 
# --enable-html-manuals and/or --enable-pdf-manuals have been set
# the generated docs are installed in pkgdocdir/doc/{html|pdf}/
#
# translations only have .po files (should also have figures/ subdirectory)
# so do not generate HTML/PDF documents for them
#
# an alternative for generating HTML manual is
# gnome-doc-tool html -o xhtml -d0 nautilus-actions-config-tool.xml
# which is part of gnome-doc-utils package
# - xhtml subdirectory has to be previously created
# - figures subdirectory has to be copied to xhtml/
# - there is (as of 2010-09-03) no icon for notes and tips
# the produced output is very close of that Yelp gives, with in particular
# summaries in each chapter while db2html does not produce them
#
# db2html, gnome-doc-tool: external links are not honored
# (converted to simple links which target the same frame window)

pkgdocdir = $(datarootdir)/doc/@PACKAGE@-@VERSION@

all-local: all-manuals

all-manuals: all-html-manuals all-pdf-manuals

install-data-local: install-manuals

install-manuals: install-html-manuals install-pdf-manuals

uninstall-local: uninstall-manuals

uninstall-manuals: uninstall-html-manuals uninstall-pdf-manuals

dist-hook: doc-dist-hook dist-html-manuals-hook dist-pdf-manuals-hook

if ENABLE_HTML_MANUALS
all-html-manuals: html.stamp omf2build.stamp

html.stamp: $(_DOC_C_DOCS) $(_DOC_C_FIGURES) $(_DOC_LC_DOCS) ChangeLog
	@-for i in C $(_DOC_REAL_LINGUAS); do \
		rm -fr $$i/$(DOC_MODULE).junk; \
		echo "Generating $$i/$(DOC_MODULE)/$(DOC_MODULE).html ..."; \
		if ! test -d $$i; then $(MKDIR_P) $$i; fi; \
		(cd $$i; \
		 if test "x$(abs_srcdir)" != "x$(abs_builddir)"; then cp $(abs_srcdir)/$$i/*.xml .; fi; \
		 db2html --nochunks $(DOC_MODULE).xml; \
		 if test "x$(abs_srcdir)" != "x$(abs_builddir)"; then rm -f *.xml; fi; \
		); \
		cp -rp $(abs_srcdir)/$$i/figures $$i/$(DOC_MODULE)/; \
		chmod -R u+w $$i/$(DOC_MODULE); \
	done
	touch html.stamp

# if --htmldir has not been specified, then we default to PKGDOCDIR/html
# in all case, we append the locale as a subdirectory level
install-html-manuals:
	@-for _lc in C $(_DOC_REAL_LINGUAS); do \
		_srcdir=$(top_builddir)/doc/nact/$$_lc/$(DOC_MODULE); \
		if test "$(htmldir)" = "$(pkgdocdir)"; then \
			_destdir=$(DESTDIR)$(pkgdocdir)/html/$$_lc; \
		else \
			_destdir=$(DESTDIR)$(htmldir)/$$_lc; \
		fi \
		if ! test -d $$_destdir; then $(MKDIR_P) $$_destdir; fi; \
		find $$_srcdir -type f | while read _f; do \
			_dname=`dirname $$_f`; \
			_subdir=`echo "$$_dname" | sed "s|^$$_srcdir[/]*||"`; \
			if ! test -d "$$_destdir/$$_subdir"; then $(MKDIR_P) "$$_destdir/$$_subdir"; fi; \
			_bname=`basename "$$_f"`; \
			echo $(INSTALL_DATA) "$$_f" "$$_destdir/$$_subdir/$$_bname"; \
			$(INSTALL_DATA) "$$_f" "$$_destdir/$$_subdir/$$_bname"; \
		done; \
	done

uninstall-html-manuals:
	rm -fr $(DESTDIR)$(pkgdocdir)/html

dist-html-manuals-hook:
	for _lc in C $(_DOC_REAL_LINGUAS); do \
		$(MKDIR_P) $(distdir)/html/$$_lc; \
		cp -r $(top_builddir)/doc/nact/$$_lc/$(DOC_MODULE)/* $(distdir)/html/$$_lc/; \
	done
	find $(distdir)/html -type d -exec chmod 755 {} \;
	find $(distdir)/html -type f -exec chmod 644 {} \;
else
all-html-manuals:

install-html-manuals:

uninstall-html-manuals:

dist-html-manuals-hook:
endif

if ENABLE_PDF_MANUALS
all-pdf-manuals: pdf.stamp omf2build.stamp

pdf.stamp: $(_DOC_C_DOCS) $(_DOC_C_FIGURES) $(_DOC_LC_DOCS) ChangeLog
	@-for i in C $(_DOC_REAL_LINGUAS); do \
		_target=$$i/$(DOC_MODULE).pdf; \
		echo "Generating $$_target ..."; \
		if ! test -d $$i; then $(MKDIR_P) $$i; fi; \
		echo "dblatex --output $$_target $(top_srcdir)/doc/nact/$$i/$(DOC_MODULE).xml"; \
		dblatex --output $$_target $(top_srcdir)/doc/nact/$$i/$(DOC_MODULE).xml; \
	done
	touch pdf.stamp

# if --pdfdir has not been specified, then we default to PKGDOCDIR/pdf
# in all case, we append the locale as a subdirectory level
install-pdf-manuals:
	@-for _lc in C $(_DOC_REAL_LINGUAS); do \
		_srcdir=$(top_builddir)/doc/nact/$$_lc; \
		if test "$(pdfdir)" = "$(pkgdocdir)"; then \
			_destdir=$(DESTDIR)$(pkgdocdir)/pdf/$$_lc; \
		else \
			_destdir=$(DESTDIR)$(pdfdir)/$$_lc; \
		fi \
		if ! test -d $$_destdir; then $(MKDIR_P) $$_destdir; fi; \
		echo $(INSTALL_DATA) "$$_srcdir/$(DOC_MODULE).pdf" "$$_destdir/$(DOC_MODULE).pdf"; \
		$(INSTALL_DATA) "$$_srcdir/$(DOC_MODULE).pdf" "$$_destdir/$(DOC_MODULE).pdf"; \
	done

uninstall-pdf-manuals:
	rm -fr $(DESTDIR)$(pkgdocdir)/pdf

dist-pdf-manuals-hook:
	for _lc in C $(_DOC_REAL_LINGUAS); do \
		$(MKDIR_P) $(distdir)/pdf/$$_lc; \
		cp $(top_builddir)/doc/nact/$$_lc/$(DOC_MODULE).pdf $(distdir)/pdf/$$_lc/; \
	done
	find $(distdir)/pdf -type d -exec chmod 755 {} \;
	find $(distdir)/pdf -type f -exec chmod 644 {} \;
else
all-pdf-manuals:

install-pdf-manuals:

uninstall-pdf-manuals:

dist-pdf-manuals-hook:
endif

# apparently, a bug in gnome-doc-utils.make
# when generating the omf for locales, original (C) xml is rightly searched
# for in srcdir, while locale (generated) ones are searched for in current
# directory, which happens to be builddir (in particular when making distcheck)
# cf. '$(_DOC_POFILES):' target in gnome-doc-utils.make to see the generation
# of locale xml
# so we copy the locale xml here from srcdir to builddir
omf2build.stamp:
	@-for _lc in $(_DOC_REAL_LINGUAS); do \
		_srcdir=$(top_srcdir)/doc/nact/$$_lc; \
		_destdir=$(top_builddir)/doc/nact/$$_lc; \
		if test "$$_destdir" != "$$_srcdir"; then \
			if test -f $$_srcdir/$(DOC_MODULE).xml; then \
				cp -v $$_srcdir/$(DOC_MODULE).xml  $$_destdir/$(DOC_MODULE).xml; \
			fi \
		fi \
	done
	touch omf2build.stamp

clean-local: clean-html-manuals clean-pdf-manuals
	rm -f *~
	rm -f *.stamp

distclean-local: clean-html-manuals clean-pdf-manuals

clean-html-manuals:
	for lc in C $(_DOC_REAL_LINGUAS); do \
		rm -fr $$lc/$(DOC_MODULE); \
		rm -fr $$lc/$(DOC_MODULE).junk; \
	done

clean-pdf-manuals:
	for lc in C $(_DOC_REAL_LINGUAS); do \
		rm -f $$lc/*.pdf; \
	done
