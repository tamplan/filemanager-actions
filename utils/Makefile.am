bin_PROGRAMS = nautilus-actions-new-config \
		nautilus-actions-check-actions-change

INCLUDES = $(NAUTILUS_ACTIONS_UTILS_CFLAGS) \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"     

nautilus_actions_new_config_SOURCES =	\
	nautilus-actions-tools-utils.c	\
	nautilus-actions-tools-utils.h	\
	nautilus-actions-new-config.c 				

nautilus_actions_new_config_LDADD =					\
	$(top_builddir)/libnautilus-actions/libnautilus-actions.la	\
	$(NAUTILUS_ACTIONS_UTILS_LIBS)

nautilus_actions_check_actions_change_SOURCES = \
	nautilus-actions-check-actions-change.c

nautilus_actions_check_actions_change_LDADD =	\
	$(NAUTILUS_ACTIONS_LIBS)		\
	$(top_builddir)/libnautilus-actions/libnautilus-actions.la
