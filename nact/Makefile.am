bin_SCRIPTS = nautilus-actions-config

gladedir = $(datadir)/nautilus-actions/nact
glade_DATA = nautilus-actions-config.glade

pixmapsdir = $(datadir)/pixmaps
pixmaps_DATA = nautilus-actions-icon.png

gladepixmapsdir = $(datadir)/nautilus-actions/nact
gladepixmaps_DATA = nautilus-actions-icon.png

Applicationsdir = $(datadir)/applications
Applications_in_files = nact.desktop.in
Applications_DATA = $(Applications_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

DISTCLEANFILES =		\
	nact.desktop		\
	nact.desktop.in		\
	nautilus-actions-config
