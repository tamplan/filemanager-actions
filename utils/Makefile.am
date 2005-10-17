bin_PROGRAMS = nautilus-actions-convert

INCLUDES = $(NAUTILUS_ACTIONS_UTILS_CFLAGS) \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"     

nautilus_actions_convert_SOURCES =	\
	nautilus-actions-convert.c 				

nautilus_actions_convert_LDADD =						\
	$(NAUTILUS_ACTIONS_UTILS_LIBS)					\
	$(top_builddir)/libnautilus-actions/libnautilus-actions.la
