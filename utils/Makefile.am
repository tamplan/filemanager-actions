bin_PROGRAMS = nautilus-actions-convert nautilus-actions-new-config

INCLUDES = $(NAUTILUS_ACTIONS_UTILS_CFLAGS) \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"     

nautilus_actions_convert_SOURCES =	\
	nautilus-actions-convert.c 				

nautilus_actions_convert_LDADD =						\
	$(NAUTILUS_ACTIONS_UTILS_LIBS)					\
	$(top_builddir)/libnautilus-actions/libnautilus-actions.la

nautilus_actions_new_config_SOURCES =	\
	nautilus-actions-new-config.c 				

nautilus_actions_new_config_LDADD =						\
	$(NAUTILUS_ACTIONS_UTILS_LIBS)					\
	$(top_builddir)/libnautilus-actions/libnautilus-actions.la
