configdir = $(datadir)/nautilus-actions
config_in_files = config_newaction.xml.in
config_DATA = $(config_in_files:.xml.in=.xml)
@INTLTOOL_XML_RULE@

schemasdir = $(sysconfdir)/gconf/schemas
schemas_in_files = config_newaction.schemas.in
schemas_DATA = $(schemas_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

DISTCLEANFILES = config_newaction.xml config_newaction.schemas
