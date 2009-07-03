/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009 Pierre Wieser and others (see AUTHORS)
 *
 * This Program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This Program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this Library; see the file COPYING.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place,
 * Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors:
 *   Frederic Ruaudel <grumz@grumz.net>
 *   Rodrigo Moya <rodrigo@gnome-db.org>
 *   Pierre Wieser <pwieser@trychlos.org>
 *   ... and many others (see AUTHORS)
 */

#ifndef __NACT_GCONF_SCHEMA_H__
#define __NACT_GCONF_SCHEMA_H__

/*
 * NactGConfSchema class definition.
 *
 * This is the base class for importing into and exporting from GConf
 * storage subsystem.
 */

#include <glib-object.h>

#include <common/na-action.h>
#include <common/na-action-profile.h>
#include <common/na-gconf-keys.h>
#include <common/na-utils.h>

G_BEGIN_DECLS

#define NACT_GCONF_SCHEMA_TYPE					( nact_gconf_schema_get_type())
#define NACT_GCONF_SCHEMA( object )				( G_TYPE_CHECK_INSTANCE_CAST( object, NACT_GCONF_SCHEMA_TYPE, NactGConfSchema ))
#define NACT_GCONF_SCHEMA_CLASS( klass )		( G_TYPE_CHECK_CLASS_CAST( klass, NACT_GCONF_SCHEMA_TYPE, NactGConfSchemaClass ))
#define NACT_IS_GCONF_SCHEMA( object )			( G_TYPE_CHECK_INSTANCE_TYPE( object, NACT_GCONF_SCHEMA_TYPE ))
#define NACT_IS_GCONF_SCHEMA_CLASS( klass )		( G_TYPE_CHECK_CLASS_TYPE(( klass ), NACT_GCONF_SCHEMA_TYPE ))
#define NACT_GCONF_SCHEMA_GET_CLASS( object )	( G_TYPE_INSTANCE_GET_CLASS(( object ), NACT_GCONF_SCHEMA_TYPE, NactGConfSchemaClass ))

typedef struct NactGConfSchemaPrivate NactGConfSchemaPrivate;

typedef struct {
	GObject                 parent;
	NactGConfSchemaPrivate *private;
}
	NactGConfSchema;

typedef struct NactGConfSchemaClassPrivate NactGConfSchemaClassPrivate;

typedef struct {
	GObjectClass                 parent;
	NactGConfSchemaClassPrivate *private;
}
	NactGConfSchemaClass;

/* GConf XML element names
 */
#define NACT_GCONF_XML_ROOT						"gconfschemafile"
#define NACT_GCONF_XML_SCHEMA_LIST				"schemalist"
#define NACT_GCONF_XML_SCHEMA_ENTRY				"schema"
#define NACT_GCONF_XML_SCHEMA_KEY				"key"
#define NACT_GCONF_XML_SCHEMA_APPLYTO			"applyto"
#define NACT_GCONF_XML_SCHEMA_TYPE				"type"
#define NACT_GCONF_XML_SCHEMA_LOCALE			"locale"
#define NACT_GCONF_XML_SCHEMA_DFT				"default"
#define NACT_GCONF_XML_SCHEMA_LIST_TYPE			"list_type"

#define NACT_GCONF_SCHEMA_PREFIX				"/schemas"

GType nact_gconf_schema_get_type( void );

G_END_DECLS

#endif /* __NACT_GCONF_SCHEMA_H__ */
