bin_PROGRAMS = nautilus-actions-config

INCLUDES =							\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"		\
	-DGLADEDIR=\""$(datadir)/nautilus-actions/nact"\"	\
	$(NAUTILUS_ACTIONS_CFLAGS)

nautilus_actions_config_SOURCES =	\
	nact.c 				\
	nact.h 				\
	nact-utils.c		\
	nact-utils.h		\
	nact-prefs.c		\
	nact-prefs.h		\
	nact-import-export.c \
	nact-import-export.h \
	nact-profile-editor.c		\
	nact-profile-editor.h			\
	nact-editor.c			\
	nact-editor.h
nautilus_actions_config_LDADD =						\
	$(NAUTILUS_ACTIONS_LIBS)					\
	$(top_builddir)/libnautilus-actions/libnautilus-actions.la

gladedir = $(datadir)/nautilus-actions/nact
glade_DATA = nautilus-actions-config.glade

Applicationsdir = $(datadir)/applications
Applications_in_files = nact.desktop.in
Applications_DATA = $(Applications_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

DISTCLEANFILES =		\
	nact.desktop
