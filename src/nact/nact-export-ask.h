/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009-2012 Pierre Wieser and others (see AUTHORS)
 *
 * Nautilus-Actions is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Nautilus-Actions is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nautilus-Actions; see the file COPYING. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Frederic Ruaudel <grumz@grumz.net>
 *   Rodrigo Moya <rodrigo@gnome-db.org>
 *   Pierre Wieser <pwieser@trychlos.org>
 *   ... and many others (see AUTHORS)
 */

#ifndef __NACT_EXPORT_ASK_H__
#define __NACT_EXPORT_ASK_H__

/**
 * SECTION: nact_export_ask
 * @short_description: #NactExportAsk class definition.
 * @include: nact/nact-export-ask.h
 *
 * This class is derived from BaseDialog.
 * It is ran each time an action is to be exported, and the user want
 * to be asked to choose the export format.
 *
 * Only actually available export formats, as provided by i/o providers,
 * are proposed here:
 * - the 'Ask' option is obviously not proposed here
 * - a 'NoImport' capability is provided by clicking on Cancel button.
 */

#include <api/na-object-item.h>

#include "base-dialog.h"

G_BEGIN_DECLS

#define NACT_TYPE_EXPORT_ASK                ( nact_export_ask_get_type())
#define NACT_EXPORT_ASK( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, NACT_TYPE_EXPORT_ASK, NactExportAsk ))
#define NACT_EXPORT_ASK_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, NACT_TYPE_EXPORT_ASK, NactExportAskClass ))
#define NACT_IS_EXPORT_ASK( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, NACT_TYPE_EXPORT_ASK ))
#define NACT_IS_EXPORT_ASK_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), NACT_TYPE_EXPORT_ASK ))
#define NACT_EXPORT_ASK_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), NACT_TYPE_EXPORT_ASK, NactExportAskClass ))

typedef struct _NactExportAskPrivate        NactExportAskPrivate;

typedef struct {
	/*< private >*/
	BaseDialog            parent;
	NactExportAskPrivate *private;
}
	NactExportAsk;

typedef struct _NactExportAskClassPrivate   NactExportAskClassPrivate;

typedef struct {
	/*< private >*/
	BaseDialogClass            parent;
	NactExportAskClassPrivate *private;
}
	NactExportAskClass;

GType  nact_export_ask_get_type( void );

gchar *nact_export_ask_user    ( BaseWindow *window, NAObjectItem *item, gboolean first );

G_END_DECLS

#endif /* __NACT_EXPORT_ASK_H__ */
