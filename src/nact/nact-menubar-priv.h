/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
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

#ifndef __NACT_MENUBAR_PRIV_H__
#define __NACT_MENUBAR_PRIV_H__

/*
 * SECTION: nact-menubar-priv
 * @title: NactMenubarPrivate
 * @short_description: The Menubar private data definition
 * @include: nact-menubar-priv.h
 *
 * This file should only be included by nact-menubar -derived files.
 */

#include <core/na-updater.h>

#include "nact-menubar.h"

G_BEGIN_DECLS

struct _NactMenubarPrivate {
	/*< private >*/
	gboolean        dispose_has_run;

	/* set at instanciation time
	 */
	BaseWindow     *window;

	/* set at initialization time
	 */
	GtkUIManager   *ui_manager;
	GtkActionGroup *action_group;
	NAUpdater      *updater;
	gboolean        is_level_zero_writable;
	gboolean        has_writable_providers;

	/* set when the selection changes
	 */
	guint           count_selected;
	GList          *selected_items;
	gboolean        is_parent_writable;		/* new menu/new action/paste menu or action */
	gboolean        enable_new_profile;		/* new profile/paste a profile */
	gboolean        is_action_writable;
	gboolean        are_parents_writable;	/* cut/delete */

	/* *** */
	gint            selected_menus;
	gint            selected_actions;
	gint            selected_profiles;
	gint            clipboard_menus;
	gint            clipboard_actions;
	gint            clipboard_profiles;
	gint            list_menus;
	gint            list_actions;
	gint            list_profiles;
	gboolean        is_modified;
	gboolean        have_exportables;
	gboolean        treeview_has_focus;
	gboolean        level_zero_order_changed;
	gulong          popup_handler;
	/* *** */
};

/* Signal emitted by the NactMenubar object on itself
 */
#define MENUBAR_SIGNAL_UPDATE_SENSITIVITIES		"menubar-signal-update-sensitivities"

/* Convenience macros to get a NactMenubar from a BaseWindow
 */
#define WINDOW_DATA_MENUBAR						"window-data-menubar"

#define BAR_WINDOW_VOID( window ) \
		g_return_if_fail( BASE_IS_WINDOW( window )); \
		NactMenubar *bar = ( NactMenubar * ) g_object_get_data( G_OBJECT( window ), WINDOW_DATA_MENUBAR ); \
		g_return_if_fail( NACT_IS_MENUBAR( bar ));

#define BAR_WINDOW_VALUE( window, value ) \
		g_return_val_if_fail( BASE_IS_WINDOW( window ), value ); \
		NactMenubar *bar = ( NactMenubar * ) g_object_get_data( G_OBJECT( window ), WINDOW_DATA_MENUBAR ); \
		g_return_val_if_fail( NACT_IS_MENUBAR( bar ), value );

/* These functions should only be called from a nact-menubar-derived file
 */
void nact_menubar_enable_item( const NactMenubar *bar, const gchar *name, gboolean enabled );

void nact_menubar_edit_on_update_sensitivities( const NactMenubar *bar );

void nact_menubar_edit_on_cut          ( GtkAction *action, NactMainWindow *window );
void nact_menubar_edit_on_copy         ( GtkAction *action, NactMainWindow *window );
void nact_menubar_edit_on_paste        ( GtkAction *action, NactMainWindow *window );
void nact_menubar_edit_on_paste_into   ( GtkAction *action, NactMainWindow *window );
void nact_menubar_edit_on_duplicate    ( GtkAction *action, NactMainWindow *window );
void nact_menubar_edit_on_delete       ( GtkAction *action, NactMainWindow *window );
void nact_menubar_edit_on_reload       ( GtkAction *action, NactMainWindow *window );
void nact_menubar_edit_on_prefererences( GtkAction *action, NactMainWindow *window );

void nact_menubar_file_on_update_sensitivities( const NactMenubar *bar );

void nact_menubar_file_on_new_menu   ( GtkAction *action, NactMainWindow *window );
void nact_menubar_file_on_new_action ( GtkAction *action, NactMainWindow *window );
void nact_menubar_file_on_new_profile( GtkAction *action, NactMainWindow *window );
void nact_menubar_file_on_save       ( GtkAction *action, NactMainWindow *window );
void nact_menubar_file_on_quit       ( GtkAction *action, NactMainWindow *window );

void nact_menubar_file_save_items      ( NactMainWindow *window );
void nact_menubar_file_install_autosave( NactMainWindow *window );

void nact_menubar_help_on_update_sensitivities( const NactMenubar *bar );

void nact_menubar_help_on_help ( GtkAction *action, NactMainWindow *window );
void nact_menubar_help_on_about( GtkAction *action, NactMainWindow *window );

void nact_menubar_maintainer_on_update_sensitivities( const NactMenubar *bar );

void nact_menubar_maintainer_on_dump_selection       ( GtkAction *action, NactMainWindow *window );
void nact_menubar_maintainer_on_brief_tree_store_dump( GtkAction *action, NactMainWindow *window );
void nact_menubar_maintainer_on_list_modified_items  ( GtkAction *action, NactMainWindow *window );
void nact_menubar_maintainer_on_dump_clipboard       ( GtkAction *action, NactMainWindow *window );

void nact_menubar_tools_on_update_sensitivities( const NactMenubar *bar );

void nact_menubar_tools_on_import( GtkAction *action, NactMainWindow *window );
void nact_menubar_tools_on_export( GtkAction *action, NactMainWindow *window );

void nact_menubar_view_on_update_sensitivities( const NactMenubar *bar );

void nact_menubar_view_on_expand_all   ( GtkAction *action, NactMainWindow *window );
void nact_menubar_view_on_collapse_all ( GtkAction *action, NactMainWindow *window );
void nact_menubar_view_on_toolbar_file ( GtkToggleAction *action, NactMainWindow *window );
void nact_menubar_view_on_toolbar_edit ( GtkToggleAction *action, NactMainWindow *window );
void nact_menubar_view_on_toolbar_tools( GtkToggleAction *action, NactMainWindow *window );
void nact_menubar_view_on_toolbar_help ( GtkToggleAction *action, NactMainWindow *window );

G_END_DECLS

#endif /* __NACT_MENUBAR_PRIV_H__ */
