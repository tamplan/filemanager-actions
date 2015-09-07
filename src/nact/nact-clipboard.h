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

#ifndef __NACT_CLIPBOARD_H__
#define __NACT_CLIPBOARD_H__

/*
 * SECTION: nact_clipboard.
 * @short_description: #NactClipboard class definition.
 * @include: nact/nact-clipboard.h
 *
 * This is just a convenience class to extract clipboard functions
 * from main window code. There is a unique object which manages all
 * clipboard buffers.
 */

#include <gtk/gtk.h>

#include "nact-main-window-def.h"

G_BEGIN_DECLS

#define NACT_TYPE_CLIPBOARD                ( nact_clipboard_get_type())
#define NACT_CLIPBOARD( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, NACT_TYPE_CLIPBOARD, NactClipboard ))
#define NACT_CLIPBOARD_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, NACT_TYPE_CLIPBOARD, NactClipboardClass ))
#define NACT_IS_CLIPBOARD( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, NACT_TYPE_CLIPBOARD ))
#define NACT_IS_CLIPBOARD_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), NACT_TYPE_CLIPBOARD ))
#define NACT_CLIPBOARD_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), NACT_TYPE_CLIPBOARD, NactClipboardClass ))

typedef struct _NactClipboardPrivate       NactClipboardPrivate;

typedef struct {
	/*< private >*/
	GObject               parent;
	NactClipboardPrivate *private;
}
	NactClipboard;

typedef struct _NactClipboardClassPrivate  NactClipboardClassPrivate;

typedef struct {
	/*< private >*/
	GObjectClass               parent;
	NactClipboardClassPrivate *private;
}
	NactClipboardClass;

/* drag and drop formats
 */
enum {
	NACT_XCHANGE_FORMAT_NACT = 0,
	NACT_XCHANGE_FORMAT_XDS,
	NACT_XCHANGE_FORMAT_APPLICATION_XML,
	NACT_XCHANGE_FORMAT_TEXT_PLAIN,
	NACT_XCHANGE_FORMAT_URI_LIST
};

/* mode indicator
 */
enum {
	CLIPBOARD_MODE_CUT = 1,
	CLIPBOARD_MODE_COPY
};

GType          nact_clipboard_get_type      ( void );

NactClipboard *nact_clipboard_new           ( NactMainWindow *window );

void           nact_clipboard_dnd_set       ( NactClipboard *clipboard,
													guint target,
													GList *rows,
													const gchar *folder,
													gboolean copy );

GList         *nact_clipboard_dnd_get_data  ( NactClipboard *clipboard, gboolean *copy );

gchar         *nact_clipboard_dnd_get_text  ( NactClipboard *clipboard, GList *rows );

void           nact_clipboard_dnd_drag_end  ( NactClipboard *clipboard );

void           nact_clipboard_dnd_clear     ( NactClipboard *clipboard );

void           nact_clipboard_primary_set   ( NactClipboard *clipboard,
													GList *items,
													gint mode );

GList         *nact_clipboard_primary_get   ( NactClipboard *clipboard,
													gboolean *relabel );

void           nact_clipboard_primary_counts( NactClipboard *clipboard,
													guint *actions,
													guint *profiles,
													guint *menus );

void           nact_clipboard_dump          ( NactClipboard *clipboard );

G_END_DECLS

#endif /* __NACT_CLIPBOARD_H__ */
