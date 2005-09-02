bin_SCRIPTS = nautilus-actions-config

gladedir = $(datadir)/nautilus-actions
glade_DATA = nautilus-actions-config.glade

pixmpapsdir = $(datadir)/nautilus-actions
pixmpaps_DATA = nautilus-launch-icon.png

Applicationsdir = $(datadir)/applications
Applications_in_files = nact.desktop.in
Applications_DATA = $(Applications_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

DISTCLEANFILES =		\
	nact.desktop		\
	nact.desktop.in		\
	nautilus-actions-config