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

#ifndef __UI_FMA_CLIPBOARD_H__
#define __UI_FMA_CLIPBOARD_H__

/*
 * SECTION: fma_clipboard.
 * @short_description: #FMAClipboard class definition.
 * @include: ui/fma-clipboard.h
 *
 * This is just a convenience class to extract clipboard functions
 * from main window code. There is a unique object which manages all
 * clipboard buffers.
 */

#include <gtk/gtk.h>

#include "nact-main-window-def.h"

G_BEGIN_DECLS

#define FMA_TYPE_CLIPBOARD                ( fma_clipboard_get_type())
#define FMA_CLIPBOARD( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, FMA_TYPE_CLIPBOARD, FMAClipboard ))
#define FMA_CLIPBOARD_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, FMA_TYPE_CLIPBOARD, FMAClipboardClass ))
#define FMA_IS_CLIPBOARD( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, FMA_TYPE_CLIPBOARD ))
#define FMA_IS_CLIPBOARD_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), FMA_TYPE_CLIPBOARD ))
#define FMA_CLIPBOARD_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), FMA_TYPE_CLIPBOARD, FMAClipboardClass ))

typedef struct _FMAClipboardPrivate       FMAClipboardPrivate;

typedef struct {
	/*< private >*/
	GObject              parent;
	FMAClipboardPrivate *private;
}
	FMAClipboard;

typedef struct _FMAClipboardClassPrivate  FMAClipboardClassPrivate;

typedef struct {
	/*< private >*/
	GObjectClass              parent;
	FMAClipboardClassPrivate *private;
}
	FMAClipboardClass;

/* drag and drop formats
 */
enum {
	FMA_XCHANGE_FORMAT_NACT = 0,
	FMA_XCHANGE_FORMAT_XDS,
	FMA_XCHANGE_FORMAT_APPLICATION_XML,
	FMA_XCHANGE_FORMAT_TEXT_PLAIN,
	FMA_XCHANGE_FORMAT_URI_LIST
};

/* mode indicator
 */
enum {
	CLIPBOARD_MODE_CUT = 1,
	CLIPBOARD_MODE_COPY
};

GType          fma_clipboard_get_type      ( void );

FMAClipboard *fma_clipboard_new           ( NactMainWindow *window );

void           fma_clipboard_dnd_set       ( FMAClipboard *clipboard,
													guint target,
													GList *rows,
													const gchar *folder,
													gboolean copy );

GList         *fma_clipboard_dnd_get_data  ( FMAClipboard *clipboard, gboolean *copy );

gchar         *fma_clipboard_dnd_get_text  ( FMAClipboard *clipboard, GList *rows );

void           fma_clipboard_dnd_drag_end  ( FMAClipboard *clipboard );

void           fma_clipboard_dnd_clear     ( FMAClipboard *clipboard );

void           fma_clipboard_primary_set   ( FMAClipboard *clipboard,
													GList *items,
													gint mode );

GList         *fma_clipboard_primary_get   ( FMAClipboard *clipboard,
													gboolean *relabel );

void           fma_clipboard_primary_counts( FMAClipboard *clipboard,
													guint *actions,
													guint *profiles,
													guint *menus );

void           fma_clipboard_dump          ( FMAClipboard *clipboard );

G_END_DECLS

#endif /* __UI_FMA_CLIPBOARD_H__ */
