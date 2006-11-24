bin_PROGRAMS = nautilus-actions-convert nautilus-actions-new-config

INCLUDES = $(NAUTILUS_ACTIONS_UTILS_CFLAGS) \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"     

nautilus_actions_convert_SOURCES =	\
	nautilus-actions-tools-utils.c	\
	nautilus-actions-tools-utils.h	\
	nautilus-actions-convert.c 				

nautilus_actions_convert_LDADD =						\
	$(top_builddir)/libnautilus-actions/libnautilus-actions.la \
	$(NAUTILUS_ACTIONS_UTILS_LIBS)

nautilus_actions_new_config_SOURCES =	\
	nautilus-actions-tools-utils.c	\
	nautilus-actions-tools-utils.h	\
	nautilus-actions-new-config.c 				

nautilus_actions_new_config_LDADD =						\
	$(top_builddir)/libnautilus-actions/libnautilus-actions.la \
	$(NAUTILUS_ACTIONS_UTILS_LIBS)
