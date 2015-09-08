/*
 * FileManager-Actions
 * A file-manager extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009-2015 Pierre Wieser and others (see AUTHORS)
 *
 * FileManager-Actions is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * FileManager-Actions is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FileManager-Actions; see the file COPYING. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Frederic Ruaudel <grumz@grumz.net>
 *   Rodrigo Moya <rodrigo@gnome-db.org>
 *   Pierre Wieser <pwieser@trychlos.org>
 *   ... and many others (see AUTHORS)
 */

#ifndef __IO_XML_FMA_XML_FORMATS_H__
#define __IO_XML_FMA_XML_FORMATS_H__

#include <glib.h>

#include <api/fma-iexporter.h>

G_BEGIN_DECLS

#define FMA_XML_FORMAT_GCONF_SCHEMA_V1			"GConfSchemaV1"
#define FMA_XML_FORMAT_GCONF_SCHEMA_V2			"GConfSchemaV2"
#define FMA_XML_FORMAT_GCONF_ENTRY				"GConfEntry"

GList *fma_xml_formats_get_formats ( const FMAIExporter *exporter );
void   fma_xml_formats_free_formats( GList *format_list );

G_END_DECLS

#endif /* __IO_XML_FMA_XML_FORMATS_H__ */
