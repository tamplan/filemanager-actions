bin_PROGRAMS = nautilus-actions-config

INCLUDES =							\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"		\
	-DGLADEDIR=\""$(datadir)/nautilus-actions/nact"\"	\
	$(NAUTILUS_ACTIONS_CFLAGS)

nautilus_actions_config_SOURCES =	\
	nact.c 				\
	nact-utils.c		\
	nact-utils.h		\
	nact-editor.c			\
	nact-editor.h
nautilus_actions_config_LDADD =						\
	$(NAUTILUS_ACTIONS_LIBS)					\
	$(top_builddir)/libnautilus-actions/libnautilus-actions.la

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
	nact.desktop
