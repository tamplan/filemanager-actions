/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu pivots.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010, 2011 Pierre Wieser and others (see AUTHORS)
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

#ifndef __CORE_NA_PIVOT_H__
#define __CORE_NA_PIVOT_H__

/* @title: NAPivot
 * @short_description: The #NAPivot Class Definition
 * @include: core/na-pivot.h
 *
 * A consuming program should allocate one new NAPivot object in its
 * startup phase. The class takes care of declaring the I/O interfaces,
 * while registering the known providers.
 * 		NAPivot *pivot = na_pivot_new();
 *
 * With this newly allocated #NAPivot object, the consuming program
 * is then able to ask for loading the items.
 * 		na_pivot_load_items( pivot, PIVOT_LOADABLE_SET );
 *
 * Notification system.
 *
 * Each I/O storage provider should monitor modifications/deletions of
 * actions, and advertize this #NAPivot, which itself will then
 * advertize any registered consumers.
 *
 * This notification system is so a double-stage one :
 *
 * 1. When an I/O storage subsystem detects a change on an action, it
 *    should call the na_iio_provider_item_changed() function, which
 *    itself will emit the "io-provider-item-changed" signal.
 *
 *    All these signals are catched by na_pivot_on_item_changed_handler()
 *
 *    notify #NAPivot of this change. The user data associated with the
 *    message is the internal id of the #NAObjectItem-derived modified
 *    object.
 *
 *    When this signal is received, #NAPivot updates accordingly the
 *    list of actions it maintains.
 *
 *    It is up to the I/O storage provider to decide if it sends a
 *    message for each and every one detected modification, or if it
 *    sends only one message for a whole, maybe coherent, set of
 *    updates.
 *
 * 2. When #NAPivot has successfully updated its list of actions, it
 *    notifies its consumers in order they update themselves.
 *
 *    Note that #NAPivot tries to factorize notification messages, and
 *    to notify its consumers only once even if it has itself received
 *    many elementary notifications from the underlying I/O storage
 *    subsystems.
 */

#include <api/na-iio-provider.h>
#include <api/na-object-api.h>

#include "na-ipivot-consumer.h"
#include "na-settings.h"

G_BEGIN_DECLS

#define NA_PIVOT_TYPE                ( na_pivot_get_type())
#define NA_PIVOT( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, NA_PIVOT_TYPE, NAPivot ))
#define NA_PIVOT_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, NA_PIVOT_TYPE, NAPivotClass ))
#define NA_IS_PIVOT( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, NA_PIVOT_TYPE ))
#define NA_IS_PIVOT_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), NA_PIVOT_TYPE ))
#define NA_PIVOT_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), NA_PIVOT_TYPE, NAPivotClass ))

typedef struct _NAPivotPrivate       NAPivotPrivate;

typedef struct {
	/*< private >*/
	GObject         parent;
	NAPivotPrivate *private;
}
	NAPivot;

typedef struct _NAPivotClassPrivate  NAPivotClassPrivate;

typedef struct {
	/*< private >*/
	GObjectClass         parent;
	NAPivotClassPrivate *private;
}
	NAPivotClass;

GType    na_pivot_get_type( void );

/* properties
 */
#define PIVOT_PROP_TREE							"pivot-prop-tree"

/* signals
 *
 * NAPivot acts as a 'sumarrizing' proxy for signals emitted by the NAIIOProvider
 * providers when they detect a modification of their underlying items storage
 * subsystems.
 * As several to many signals may be emitted when such a modification occurs,
 * NAPivot summarizes all these signals in an only one 'items-changed' event.
 */
#define PIVOT_SIGNAL_ITEMS_CHANGED				"pivot-items-changed"

/* Loadable population
 * NACT management user interface defaults to PIVOT_LOAD_ALL
 * N-A plugin set the loadable population to !PIVOT_LOAD_DISABLED & !PIVOT_LOAD_INVALID
 */
typedef enum {
	PIVOT_LOAD_NONE     = 0,
	PIVOT_LOAD_DISABLED = 1 << 0,
	PIVOT_LOAD_INVALID  = 1 << 1,
	PIVOT_LOAD_ALL      = 0xff
}
	NAPivotLoadableSet;

NAPivot      *na_pivot_new ( void );
void          na_pivot_dump( const NAPivot *pivot );

/* Management of the plugins which claim to implement a Nautilus-Actions interface.
 * As of 2.30, these are NAIIOProvider, NAIImporter, NAIExporter
 */
GList        *na_pivot_get_providers ( const NAPivot *pivot, GType type );
void          na_pivot_free_providers( GList *providers );

/* Items, menus and actions, management
 */
NAObjectItem *na_pivot_get_item     ( const NAPivot *pivot, const gchar *id );
GList        *na_pivot_get_items    ( const NAPivot *pivot );
void          na_pivot_load_items   ( NAPivot *pivot );
void          na_pivot_set_new_items( NAPivot *pivot, GList *tree );

void          na_pivot_on_item_changed_handler( NAIIOProvider *provider, NAPivot *pivot  );

/* NAIPivotConsumer interface management
 * to be deprecated
 */
void          na_pivot_register_consumer( NAPivot *pivot, const NAIPivotConsumer *consumer );

/*
 * Monitoring and preferences management
 */

NASettings   *na_pivot_get_settings     ( const NAPivot *pivot );

/* NAPivot properties and configuration
 */
void          na_pivot_set_automatic_reload            ( NAPivot *pivot, gboolean reload );
void          na_pivot_set_loadable                    ( NAPivot *pivot, guint loadable );

gboolean      na_pivot_is_configuration_locked_by_admin( const NAPivot *pivot );

G_END_DECLS

#endif /* __CORE_NA_PIVOT_H__ */
