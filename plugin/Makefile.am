INCLUDES =				\
	$(NAUTILUS_ACTIONS_CFLAGS)

nautilus_extensiondir = $(NAUTILUS_EXTENSIONS_DIR)
nautilus_extension_LTLIBRARIES = libnautilus-actions.la

libnautilus_actions_la_SOURCES = 	\
	nautilus-actions.c		\
	nautilus-actions.h		\
	nautilus-actions-module.c	\
	nautilus-actions-test.c		\
	nautilus-actions-test.h		\
	nautilus-actions-utils.c	\
	nautilus-actions-utils.h

libnautilus_actions_la_LDFLAGS = -module -avoid-version
libnautilus_actions_la_LIBADD = $(NAUTILUS_ACTIONS_LIBS) \
	$(top_builddir)/libnautilus-actions/libnautilus-actions.la
