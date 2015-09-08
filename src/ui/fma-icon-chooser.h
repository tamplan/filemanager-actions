/*
 * Nautilus Actions
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

#ifndef __UI_FMA_ICON_CHOOSER_H__
#define __UI_FMA_ICON_CHOOSER_H__

/**
 * SECTION: fma_icon_chooser
 * @short_description: FMAIconChooser dialog box
 * @include: ui/fma-icon-chooser.h
 *
 * This class is derived from BaseDialog.
 */

#include "base-dialog.h"
#include "fma-main-window-def.h"

G_BEGIN_DECLS

#define FMA_TYPE_ICON_CHOOSER                ( fma_icon_chooser_get_type())
#define FMA_ICON_CHOOSER( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, FMA_TYPE_ICON_CHOOSER, FMAIconChooser ))
#define FMA_ICON_CHOOSER_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, FMA_TYPE_ICON_CHOOSER, FMAIconChooserClass ))
#define FMA_IS_ICON_CHOOSER( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, FMA_TYPE_ICON_CHOOSER ))
#define FMA_IS_ICON_CHOOSER_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), FMA_TYPE_ICON_CHOOSER ))
#define FMA_ICON_CHOOSER_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), FMA_TYPE_ICON_CHOOSER, FMAIconChooserClass ))

typedef struct _FMAIconChooserPrivate        FMAIconChooserPrivate;

typedef struct {
	/*< private >*/
	BaseDialog             parent;
	FMAIconChooserPrivate *private;
}
	FMAIconChooser;

typedef struct _FMAIconChooserClassPrivate   FMAIconChooserClassPrivate;

typedef struct {
	/*< private >*/
	BaseDialogClass             parent;
	FMAIconChooserClassPrivate *private;
}
	FMAIconChooserClass;

GType  fma_icon_chooser_get_type   ( void );

gchar *fma_icon_chooser_choose_icon( FMAMainWindow *main_window, const gchar *icon_name );

G_END_DECLS

#endif /* __UI_FMA_ICON_CHOOSER_H__ */
