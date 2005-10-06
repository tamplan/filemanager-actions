noinst_LTLIBRARIES = libnautilus-actions.la

INCLUDES =						\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	$(NAUTILUS_ACTIONS_CFLAGS)

libnautilus_actions_la_SOURCES = 	\
	nautilus-actions-config.c 	\
	nautilus-actions-config.h  \
	nautilus-actions-config-gconf.c \
	nautilus-actions-config-gconf.h \
	nautilus-actions-config-xml.c	\
	nautilus-actions-config-xml.h
libnautilus_actions_la_LIBADD = -luuid
