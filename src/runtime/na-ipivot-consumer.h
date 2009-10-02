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

#ifndef __NA_IPIVOT_CONSUMER_H__
#define __NA_IPIVOT_CONSUMER_H__

/**
 * SECTION: na_ipivot_consumer
 * @short_description: #NAIPivotConsumer interface definition.
 * @include: runtime/na-ipivot-consumer.h
 *
 * This interface should be implemented by all classes which embed a
 * #NAPivot object, in order to receive modification notification
 * messages.
 */

#include <glib-object.h>

G_BEGIN_DECLS

#define NA_IPIVOT_CONSUMER_TYPE							( na_ipivot_consumer_get_type())
#define NA_IPIVOT_CONSUMER( object )					( G_TYPE_CHECK_INSTANCE_CAST( object, NA_IPIVOT_CONSUMER_TYPE, NAIPivotConsumer ))
#define NA_IS_IPIVOT_CONSUMER( object )					( G_TYPE_CHECK_INSTANCE_TYPE( object, NA_IPIVOT_CONSUMER_TYPE ))
#define NA_IPIVOT_CONSUMER_GET_INTERFACE( instance )	( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), NA_IPIVOT_CONSUMER_TYPE, NAIPivotConsumerInterface ))

typedef struct NAIPivotConsumer NAIPivotConsumer;

typedef struct NAIPivotConsumerInterfacePrivate NAIPivotConsumerInterfacePrivate;

typedef struct {
	GTypeInterface                     parent;
	NAIPivotConsumerInterfacePrivate *private;

	/**
	 * on_actions_changed:
	 * @instance: the #NAIPivotConsumer instance which implements this
	 * interface.
	 * user_data: user data set when emitting the message. Currently,
	 * not used.
	 *
	 * This function is triggered once when #NAPivot detects the end of
	 * a bunch of modifications. At this time, the embedded list of
	 * #NAAction has been updated to be up to date.
	 */
	void ( *on_actions_changed )      ( NAIPivotConsumer *instance, gpointer user_data );

	/**
	 * on_display_about_changed:
	 * @instance: the #NAIPivotConsumer instance which implements this
	 * interface.
	 * @order_mode: the new order mode.
	 *
	 * This function is triggered each time the setting of the display
	 * of an 'About' item in the Nautilus context menu is changed.
	 */
	void ( *on_display_about_changed )( NAIPivotConsumer *instance, gint order_mode );

	/**
	 * on_display_order_changed:
	 * @instance: the #NAIPivotConsumer instance which implements this
	 * interface.
	 * @enabled: whether the 'About' item should be enabled.
	 *
	 * This function is triggered each time the display order preference
	 * is changed.
	 */
	void ( *on_display_order_changed )( NAIPivotConsumer *instance, gboolean enabled );
}
	NAIPivotConsumerInterface;

GType na_ipivot_consumer_get_type( void );

void  na_ipivot_consumer_delay_notify( NAIPivotConsumer *instance );

void  na_ipivot_consumer_notify_actions_changed( NAIPivotConsumer *instance );
void  na_ipivot_consumer_notify_of_display_order_change( NAIPivotConsumer *instance, gint order_mode );
void  na_ipivot_consumer_notify_of_display_about_change( NAIPivotConsumer *instance, gboolean enabled );

G_END_DECLS

#endif /* __NA_IPIVOT_CONSUMER_H__ */
