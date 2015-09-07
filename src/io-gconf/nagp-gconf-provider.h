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

#ifndef __NAGP_GCONF_PROVIDER_H__
#define __NAGP_GCONF_PROVIDER_H__

/**
 * SECTION: nagp_gconf_provider
 * @short_description: #NagpGConfProvider class definition.
 * @include: na-gconf-provider.h
 *
 * This class manages the GConf I/O storage subsystem, or, in other words,
 * the GConf subsystem as an #NAIIOProvider. As this, it should only be
 * used through the #NAIIOProvider interface.
 *
 * #NagpGConfProvider uses #NAGConfMonitor to watch at the configuration
 * tree. Modifications are notified to the #NAIIOProvider interface.
 */

#include <glib-object.h>
#include <gconf/gconf-client.h>

G_BEGIN_DECLS

#define NAGP_GCONF_PROVIDER_TYPE				( nagp_gconf_provider_get_type())
#define NAGP_GCONF_PROVIDER( object )			( G_TYPE_CHECK_INSTANCE_CAST( object, NAGP_GCONF_PROVIDER_TYPE, NagpGConfProvider ))
#define NAGP_GCONF_PROVIDER_CLASS( klass )		( G_TYPE_CHECK_CLASS_CAST( klass, NAGP_GCONF_PROVIDER_TYPE, NagpGConfProviderClass ))
#define NAGP_IS_GCONF_PROVIDER( object )		( G_TYPE_CHECK_INSTANCE_TYPE( object, NAGP_GCONF_PROVIDER_TYPE ))
#define NAGP_IS_GCONF_PROVIDER_CLASS( klass )	( G_TYPE_CHECK_CLASS_TYPE(( klass ), NAGP_GCONF_PROVIDER_TYPE ))
#define NAGP_GCONF_PROVIDER_GET_CLASS( object )	( G_TYPE_INSTANCE_GET_CLASS(( object ), NAGP_GCONF_PROVIDER_TYPE, NagpGConfProviderClass ))

/* private instance data
 */
typedef struct _NagpGConfProviderPrivate {
	/*< private >*/
	gboolean     dispose_has_run;
	GConfClient *gconf;
	GList       *monitors;
	guint        event_source_id;
	GTimeVal     last_event;
}
	NagpGConfProviderPrivate;

typedef struct {
	/*< private >*/
	GObject                   parent;
	NagpGConfProviderPrivate *private;
}
	NagpGConfProvider;

typedef struct _NagpGConfProviderClassPrivate   NagpGConfProviderClassPrivate;

typedef struct {
	/*< private >*/
	GObjectClass                   parent;
	NagpGConfProviderClassPrivate *private;
}
	NagpGConfProviderClass;

GType nagp_gconf_provider_get_type     ( void );
void  nagp_gconf_provider_register_type( GTypeModule *module );

G_END_DECLS

#endif /* __NAGP_GCONF_PROVIDER_H__ */
