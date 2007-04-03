noinst_LTLIBRARIES = libnautilus-actions.la

INCLUDES =						\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	$(NAUTILUS_ACTIONS_CFLAGS)

libnautilus_actions_la_SOURCES = 	\
	nautilus-actions-config.c 	\
	nautilus-actions-config.h  \
	nautilus-actions-config-gconf.c \
	nautilus-actions-config-gconf.h \
	nautilus-actions-config-gconf-private.h \
	nautilus-actions-config-gconf-reader.c \
	nautilus-actions-config-gconf-reader.h \
	nautilus-actions-config-gconf-writer.c \
	nautilus-actions-config-gconf-writer.h \
	nautilus-actions-config-schema-writer.c \
	nautilus-actions-config-schema-writer.h \
	nautilus-actions-config-schema-reader.c \
	nautilus-actions-config-schema-reader.h 
libnautilus_actions_la_LIBADD = -lxml2 -luuid
