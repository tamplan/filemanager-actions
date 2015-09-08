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

#ifndef __UI_NACT_PREFERENCES_EDITOR_H__
#define __UI_NACT_PREFERENCES_EDITOR_H__

/**
 * SECTION: nact_preferences_editor
 * @short_description: #NactPreferencesEditor class definition.
 * @include: ui/nact-preferences-editor.h
 *
 * This class is derived from NactWindow.
 * It encapsulates the "PreferencesDialog" widget dialog.
 */

#include "base-dialog.h"
#include "fma-main-window-def.h"

G_BEGIN_DECLS

#define NACT_TYPE_PREFERENCES_EDITOR                ( nact_preferences_editor_get_type())
#define NACT_PREFERENCES_EDITOR( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, NACT_TYPE_PREFERENCES_EDITOR, NactPreferencesEditor ))
#define NACT_PREFERENCES_EDITOR_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, NACT_TYPE_PREFERENCES_EDITOR, NactPreferencesEditorClass ))
#define NACT_IS_PREFERENCES_EDITOR( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, NACT_TYPE_PREFERENCES_EDITOR ))
#define NACT_IS_PREFERENCES_EDITOR_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), NACT_TYPE_PREFERENCES_EDITOR ))
#define NACT_PREFERENCES_EDITOR_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), NACT_TYPE_PREFERENCES_EDITOR, NactPreferencesEditorClass ))

typedef struct _NactPreferencesEditorPrivate        NactPreferencesEditorPrivate;

typedef struct {
	/*< private >*/
	BaseDialog                    parent;
	NactPreferencesEditorPrivate *private;
}
	NactPreferencesEditor;

typedef struct {
	/*< private >*/
	BaseDialogClass               parent;
}
	NactPreferencesEditorClass;

GType nact_preferences_editor_get_type( void );

void  nact_preferences_editor_run     ( FMAMainWindow *main_window );

G_END_DECLS

#endif /* __UI_NACT_PREFERENCES_EDITOR_H__ */
