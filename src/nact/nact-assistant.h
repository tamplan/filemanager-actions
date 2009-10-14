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

#ifndef __NACT_ASSISTANT_H__
#define __NACT_ASSISTANT_H__

/**
 * SECTION: nact_assistant
 * @short_description: #NactAssistant class definition.
 * @include: nact/nact-assistant.h
 *
 * This class is derived from NactWindow class, and serves as a base
 * class for all Nautilus Actions assistants.
 *
 * Note: as a work-around to #589745 (Apply message in GtkAssistant),
 * we may trigger "on_assistant_apply" function from the
 * "on_prepare_message" handler.
 * The provided patch has been applied on 2009-08-07, and released in
 * Gtk+ 2.17.7. So, this work-around will can be safely removed when
 * minimal Gtk+ version will be 2.18 or later.
 */

#include "nact-window.h"

G_BEGIN_DECLS

#define NACT_ASSISTANT_TYPE					( nact_assistant_get_type())
#define NACT_ASSISTANT( object )			( G_TYPE_CHECK_INSTANCE_CAST( object, NACT_ASSISTANT_TYPE, NactAssistant ))
#define NACT_ASSISTANT_CLASS( klass )		( G_TYPE_CHECK_CLASS_CAST( klass, NACT_ASSISTANT_TYPE, NactAssistantClass ))
#define NACT_IS_ASSISTANT( object )			( G_TYPE_CHECK_INSTANCE_TYPE( object, NACT_ASSISTANT_TYPE ))
#define NACT_IS_ASSISTANT_CLASS( klass )	( G_TYPE_CHECK_CLASS_TYPE(( klass ), NACT_ASSISTANT_TYPE ))
#define NACT_ASSISTANT_GET_CLASS( object )	( G_TYPE_INSTANCE_GET_CLASS(( object ), NACT_ASSISTANT_TYPE, NactAssistantClass ))

typedef struct NactAssistantPrivate NactAssistantPrivate;

typedef struct {
	NactWindow            parent;
	NactAssistantPrivate *private;
}
	NactAssistant;

typedef struct NactAssistantClassPrivate NactAssistantClassPrivate;

typedef struct {
	NactWindowClass            parent;
	NactAssistantClassPrivate *private;

	/* api */
	gchar  * ( *get_ui_fname )         ( NactAssistant *window );
	void     ( *on_assistant_apply )   ( NactAssistant *window, GtkAssistant *assistant );
	void     ( *on_assistant_cancel )  ( NactAssistant *window, GtkAssistant *assistant );
	void     ( *on_assistant_close )   ( NactAssistant *window, GtkAssistant *assistant );
	void     ( *on_assistant_prepare ) ( NactAssistant *window, GtkAssistant *assistant, GtkWidget *page );
}
	NactAssistantClass;

GType nact_assistant_get_type( void );

void  nact_assistant_set_warn_on_cancel( NactAssistant *window, gboolean warn );

G_END_DECLS

#endif /* __NACT_ASSISTANT_H__ */
