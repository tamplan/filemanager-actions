/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu pivots.
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

#ifndef __NA_RUNTIME_PIVOT_H__
#define __NA_RUNTIME_PIVOT_H__

/**
 * SECTION: na_pivot
 * @short_description: #NAPivot class definition.
 * @include: common/na-pivot.h
 *
 * A consuming program should allocate one new NAPivot object in its
 * startup phase. The class takes care of declaring the I/O interface,
 * while registering the known providers. The object will then load
 * itself the existing list of actions.
 *
 * Notification system
 *
 * Each I/O storage provider should monitor modifications/deletions of
 * actions, and advertize this #NAPivot, which itself will then
 * advertize any registered consumers.
 *
 * This notification system is so a double-stage one :
 *
 * 1. When an I/O storage subsystem detects a change on an action, it
 *    should emit the "notify-consumer-of-action-change" signal to
 *    notify #NAPivot of this change. The user data associated with the
 *    message should be a #gpointer to a #NAPivotNotify structure.
 *
 *    When this signal is received, #NAPivot updates accordingly the
 *    list of actions it maintains.
 *
 *    It is up to the I/O storage provider to decide if it sends a
 *    message for each and every one detected modification, or if it
 *    sends only one message for a whole, maybe coherent, set of
 *    updates.
 *
 *    This first stage message is defined in na-iio-provider.h,
 *    as NA_IIO_PROVIDER_SIGNAL_ACTION_CHANGED.
 *
 * 2. When #NAPivot has successfully updated its list of actions, it
 *    notifies its consumers in order they update themselves.
 *
 *    Note that #NAPivot tries to factorize notification messages, and
 *    to notify its consumers only once even if it has itself received
 *    many elementary notifications from the underlying I/O storage
 *    subsystem.
 */

#include "na-object-class.h"
#include "na-object-id-class.h"
#include "na-object-item-class.h"
#include "na-ipivot-consumer.h"

G_BEGIN_DECLS

#define NA_PIVOT_TYPE					( na_pivot_get_type())
#define NA_PIVOT( object )				( G_TYPE_CHECK_INSTANCE_CAST( object, NA_PIVOT_TYPE, NAPivot ))
#define NA_PIVOT_CLASS( klass )			( G_TYPE_CHECK_CLASS_CAST( klass, NA_PIVOT_TYPE, NAPivotClass ))
#define NA_IS_PIVOT( object )			( G_TYPE_CHECK_INSTANCE_TYPE( object, NA_PIVOT_TYPE ))
#define NA_IS_PIVOT_CLASS( klass )		( G_TYPE_CHECK_CLASS_TYPE(( klass ), NA_PIVOT_TYPE ))
#define NA_PIVOT_GET_CLASS( object )	( G_TYPE_INSTANCE_GET_CLASS(( object ), NA_PIVOT_TYPE, NAPivotClass ))

typedef struct NAPivotPrivate NAPivotPrivate;

typedef struct {
	GObject         parent;
	NAPivotPrivate *private;
}
	NAPivot;

typedef struct NAPivotClassPrivate NAPivotClassPrivate;

typedef struct {
	GObjectClass         parent;
	NAPivotClassPrivate *private;
}
	NAPivotClass;

GType     na_pivot_get_type( void );

NAPivot  *na_pivot_new( const NAIPivotConsumer *notified );
void      na_pivot_check_status( const NAPivot *pivot );
void      na_pivot_dump( const NAPivot *pivot );

GList    *na_pivot_get_providers( const NAPivot *pivot, GType type );
void      na_pivot_free_providers( GList *providers );

GList    *na_pivot_get_items( const NAPivot *pivot );
void      na_pivot_reload_items( NAPivot *pivot );

void      na_pivot_add_item( NAPivot *pivot, const NAObject *item );
NAObject *na_pivot_get_item( const NAPivot *pivot, const gchar *uuid );
void      na_pivot_remove_item( NAPivot *pivot, NAObject *item );

guint     na_pivot_delete_item( const NAPivot *pivot, const NAObjectItem *item, GSList **messages );
guint     na_pivot_write_item( const NAPivot *pivot, NAObjectItem *item, GSList **messages );

void      na_pivot_register_consumer( NAPivot *pivot, const NAIPivotConsumer *consumer );

gboolean  na_pivot_get_automatic_reload( const NAPivot *pivot );
void      na_pivot_set_automatic_reload( NAPivot *pivot, gboolean reload );

gint      na_pivot_sort_alpha_asc( const NAObjectId *a, const NAObjectId *b );
gint      na_pivot_sort_alpha_desc( const NAObjectId *a, const NAObjectId *b );

void      na_pivot_write_level_zero( const NAPivot *pivot, GList *items );

/* data passed from the storage subsystem when an action is changed
 */
enum {
	NA_PIVOT_STR = 1,
	NA_PIVOT_BOOL,
	NA_PIVOT_STRLIST
};

typedef struct {
	gchar   *uuid;
	gchar   *profile;
	gchar   *parm;
	guint    type;
	gpointer data;
}
	NAPivotNotify;

void       na_pivot_free_notify( NAPivotNotify *data );

G_END_DECLS

#endif /* __NA_RUNTIME_PIVOT_H__ */
