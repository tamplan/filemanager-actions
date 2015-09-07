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

#ifndef __FILE_MANAGER_ACTIONS_API_OBJECT_API_H__
#define __FILE_MANAGER_ACTIONS_API_OBJECT_API_H__

/**
 * SECTION: object-api
 * @title: API
 * @short_description: The Common Public #FMAObject API
 * @include: file-manager-actions/fma-object-api.h
 *
 * We define here a common API which makes easier to write (and read)
 * the code; all object functions are named fma_object; all arguments
 * are casted directly in the macro.
 */

#include "fma-ifactory-object.h"
#include "fma-ifactory-object-data.h"
#include "fma-iduplicable.h"
#include "fma-icontext.h"
#include "fma-object-action.h"
#include "fma-object-profile.h"
#include "fma-object-menu.h"

G_BEGIN_DECLS

/* FMAIDuplicable
 */
#define fma_object_duplicate( obj, mode )                fma_iduplicable_duplicate( FMA_IDUPLICABLE( obj ), mode )
#define fma_object_check_status( obj )                   fma_object_object_check_status_rec( FMA_OBJECT( obj ))

#define fma_object_get_origin( obj )                     fma_iduplicable_get_origin( FMA_IDUPLICABLE( obj ))
#define fma_object_is_valid( obj )                       fma_iduplicable_is_valid( FMA_IDUPLICABLE( obj ))
#define fma_object_is_modified( obj )                    fma_iduplicable_is_modified( FMA_IDUPLICABLE( obj ))

#define fma_object_set_origin( obj, origin )             fma_iduplicable_set_origin( FMA_IDUPLICABLE( obj ), ( FMAIDuplicable * )( origin ))
#define fma_object_reset_origin( obj, origin )           fma_object_object_reset_origin( FMA_OBJECT( obj ), ( FMAObject * )( origin ))

/* FMAObject
 */
#define fma_object_dump( obj )                           fma_object_object_dump( FMA_OBJECT( obj ))
#define fma_object_dump_norec( obj )                     fma_object_object_dump_norec( FMA_OBJECT( obj ))
#define fma_object_dump_tree( tree )                     fma_object_object_dump_tree( tree )
#define fma_object_ref( obj )                            fma_object_object_ref( FMA_OBJECT( obj ))
#define fma_object_unref( obj )                          fma_object_object_unref( FMA_OBJECT( obj ))

#define fma_object_debug_invalid( obj, reason )          fma_object_object_debug_invalid( FMA_OBJECT( obj ), ( const gchar * )( reason ))

/* FMAObjectId
 */
#define fma_object_get_id( obj )                         (( gchar * ) fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_ID ))
#define fma_object_get_label( obj )                      (( gchar * ) fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), ( FMA_IS_OBJECT_PROFILE( obj ) ? FMAFO_DATA_DESCNAME : FMAFO_DATA_LABEL )))
#define fma_object_get_label_noloc( obj )                (( gchar * )( FMA_IS_OBJECT_PROFILE( obj ) ? fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_DESCNAME_NOLOC ) : NULL ))
#define fma_object_get_parent( obj )                     (( FMAObjectItem * ) fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_PARENT ))

#define fma_object_set_id( obj, id )                     fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_ID, ( const void * )( id ))
#define fma_object_set_label( obj, label )               fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), ( FMA_IS_OBJECT_PROFILE( obj ) ? FMAFO_DATA_DESCNAME : FMAFO_DATA_LABEL ), ( const void * )( label ))
#define fma_object_set_parent( obj, parent )             fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_PARENT, ( const void * )( parent ))

#define fma_object_sort_alpha_asc( a, b )                fma_object_id_sort_alpha_asc( FMA_OBJECT_ID( a ), FMA_OBJECT_ID( b ))
#define fma_object_sort_alpha_desc( a, b )               fma_object_id_sort_alpha_desc( FMA_OBJECT_ID( a ), FMA_OBJECT_ID( b ))

#define fma_object_prepare_for_paste( obj, relabel, renumber, parent ) \
                                                         fma_object_id_prepare_for_paste( FMA_OBJECT_ID( obj ), ( relabel ), ( renumber ), ( FMAObjectId * )( parent ))
#define fma_object_set_copy_of_label( obj )              fma_object_id_set_copy_of_label( FMA_OBJECT_ID( obj ))
#define fma_object_set_new_id( obj, parent )             fma_object_id_set_new_id( FMA_OBJECT_ID( obj ), ( FMAObjectId * )( parent ))

/* FMAObjectItem
 */
#define fma_object_get_tooltip( obj )                    (( gchar * ) fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_TOOLTIP ))
#define fma_object_get_icon( obj )                       (( gchar * ) fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_ICON ))
#define fma_object_get_icon_noloc( obj )                 (( gchar * ) fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_ICON_NOLOC ))
#define fma_object_get_description( obj )                (( gchar * ) fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_DESCRIPTION ))
#define fma_object_get_items( obj )                      (( GList * ) fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_SUBITEMS ))
#define fma_object_get_items_slist( obj )                (( GSList * ) fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_SUBITEMS_SLIST ))
#define fma_object_is_enabled( obj )                     (( gboolean ) GPOINTER_TO_UINT( fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_ENABLED )))
#define fma_object_is_readonly( obj )                    (( gboolean ) GPOINTER_TO_UINT( fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_READONLY )))
#define fma_object_get_provider( obj )                   fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_PROVIDER )
#define fma_object_get_provider_data( obj )              fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_PROVIDER_DATA )
#define fma_object_get_iversion( obj )                   GPOINTER_TO_UINT( fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_IVERSION ))
#define fma_object_get_shortcut( obj )                   (( gchar * ) fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_SHORTCUT ))

#define fma_object_set_tooltip( obj, tooltip )           fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_TOOLTIP, ( const void * )( tooltip ))
#define fma_object_set_icon( obj, icon )                 fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_ICON, ( const void * )( icon ))
#define fma_object_set_description( obj, desc )          fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_DESCRIPTION, ( const void * )( desc ))
#define fma_object_set_items( obj, list )                fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_SUBITEMS, ( const void * )( list ))
#define fma_object_set_items_slist( obj, slist )         fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_SUBITEMS_SLIST, ( const void * )( slist ))
#define fma_object_set_enabled( obj, enabled )           fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_ENABLED, ( const void * ) GUINT_TO_POINTER( enabled ))
#define fma_object_set_readonly( obj, readonly )         fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_READONLY, ( const void * ) GUINT_TO_POINTER( readonly ))
#define fma_object_set_provider( obj, provider )         fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_PROVIDER, ( const void * )( provider ))
#define fma_object_set_provider_data( obj, data )        fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_PROVIDER_DATA, ( const void * )( data ))
#define fma_object_set_iversion( obj, version )          fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_IVERSION, ( const void * ) GUINT_TO_POINTER( version ))
#define fma_object_set_shortcut( obj, shortcut )         fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_SHORTCUT, ( const void * )( shortcut ))

#define fma_object_get_item( obj, id )                   fma_object_item_get_item( FMA_OBJECT_ITEM( obj ),( const gchar * )( id ))
#define fma_object_get_position( obj, child )            fma_object_item_get_position( FMA_OBJECT_ITEM( obj ), FMA_OBJECT_ID( child ))
#define fma_object_append_item( obj, child )             fma_object_item_append_item( FMA_OBJECT_ITEM( obj ), FMA_OBJECT_ID( child ))
#define fma_object_insert_at( obj, child, pos )          fma_object_item_insert_at( FMA_OBJECT_ITEM( obj ), FMA_OBJECT_ID( child ), ( pos ))
#define fma_object_insert_item( obj, child, sibling )    fma_object_item_insert_item( FMA_OBJECT_ITEM( obj ), FMA_OBJECT_ID( child ), ( FMAObjectId * )( sibling ))
#define fma_object_remove_item( obj, child )             fma_object_item_remove_item( FMA_OBJECT_ITEM( obj ), FMA_OBJECT_ID( child ))

#define fma_object_get_items_count( obj )                fma_object_item_get_items_count( FMA_OBJECT_ITEM( obj ))
#define fma_object_count_items( list, cm, ca, cp )       fma_object_item_count_items( list, ( cm ), ( ca ), ( cp ), TRUE )
#define fma_object_copyref_items( tree )                 fma_object_item_copyref_items( tree )
#define fma_object_free_items( tree )                    fma_object_item_free_items( tree )

#define fma_object_is_finally_writable( obj, r )         fma_object_item_is_finally_writable( FMA_OBJECT_ITEM( obj ), ( r ))
#define fma_object_set_writability_status( obj, w, r )   fma_object_item_set_writability_status( FMA_OBJECT_ITEM( obj ), ( w ), ( r ))

/* FMAObjectAction
 */
#define fma_object_get_version( obj )                    (( gchar * ) fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_VERSION ))
#define fma_object_is_target_selection( obj )            (( gboolean ) GPOINTER_TO_UINT( fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_TARGET_SELECTION )))
#define fma_object_is_target_location( obj )             (( gboolean ) GPOINTER_TO_UINT( fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_TARGET_LOCATION )))
#define fma_object_is_target_toolbar( obj )              (( gboolean ) GPOINTER_TO_UINT( fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_TARGET_TOOLBAR )))
#define fma_object_get_toolbar_label( obj )              (( gchar * ) fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_TOOLBAR_LABEL ))
#define fma_object_is_toolbar_same_label( obj )          (( gboolean ) GPOINTER_TO_UINT( fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_TOOLBAR_SAME_LABEL )))
#define fma_object_get_last_allocated( obj )             (( guint ) GPOINTER_TO_UINT( fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_LAST_ALLOCATED )))

#define fma_object_set_version( obj, version )           fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_VERSION, ( const void * )( version ))
#define fma_object_set_target_selection( obj, target )   fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_TARGET_SELECTION, ( const void * ) GUINT_TO_POINTER( target ))
#define fma_object_set_target_location( obj, target )    fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_TARGET_LOCATION, ( const void * ) GUINT_TO_POINTER( target ))
#define fma_object_set_target_toolbar( obj, target )     fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_TARGET_TOOLBAR, ( const void * ) GUINT_TO_POINTER( target ))
#define fma_object_set_toolbar_label( obj, label )       fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_TOOLBAR_LABEL, ( const void * )( label ))
#define fma_object_set_toolbar_same_label( obj, same )   fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_TOOLBAR_SAME_LABEL, ( const void * ) GUINT_TO_POINTER( same ))
#define fma_object_set_last_allocated( obj, last )       fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_LAST_ALLOCATED, ( const void * ) GUINT_TO_POINTER( last ))

#define fma_object_set_last_version( obj )               fma_object_action_set_last_version( FMA_OBJECT_ACTION( obj ))
#define fma_object_reset_last_allocated( obj )           fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_LAST_ALLOCATED, ( const void * ) GUINT_TO_POINTER( 0 ))
#define fma_object_attach_profile( obj, profile )        fma_object_action_attach_profile( FMA_OBJECT_ACTION( obj ), FMA_OBJECT_PROFILE( profile ))

/* FMAObjectProfile
 */
#define fma_object_get_path( obj )                       (( gchar * ) fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_PATH ))
#define fma_object_get_parameters( obj )                 (( gchar * ) fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_PARAMETERS ))
#define fma_object_get_working_dir( obj )                (( gchar * ) fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_WORKING_DIR ))
#define fma_object_get_execution_mode( obj )             (( gchar * ) fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_EXECUTION_MODE ))
#define fma_object_get_startup_notify( obj )             (( gboolean ) GPOINTER_TO_UINT( fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_STARTUP_NOTIFY )))
#define fma_object_get_startup_class( obj )              (( gchar * ) fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_STARTUP_WMCLASS ))
#define fma_object_get_execute_as( obj )                 (( gchar * ) fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_EXECUTE_AS ))

#define fma_object_set_path( obj, path )                 fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_PATH, ( const void * )( path ))
#define fma_object_set_parameters( obj, parms )          fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_PARAMETERS, ( const void * )( parms ))
#define fma_object_set_working_dir( obj, uri )           fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_WORKING_DIR, ( const void * )( uri ))
#define fma_object_set_execution_mode( obj, mode )       fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_EXECUTION_MODE, ( const void * )( mode ))
#define fma_object_set_startup_notify( obj, notify )     fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_STARTUP_NOTIFY, ( const void * ) GUINT_TO_POINTER( notify ))
#define fma_object_set_startup_class( obj, class )       fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_STARTUP_WMCLASS, ( const void * )( class ))
#define fma_object_set_execute_as( obj, user )           fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_EXECUTE_AS, ( const void * )( user ))

/* FMAIContext
 */
#define fma_object_check_mimetypes( obj )                fma_icontext_check_mimetypes( FMA_ICONTEXT( obj ))

#define fma_object_get_basenames( obj )                  (( GSList * ) fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_BASENAMES ))
#define fma_object_get_matchcase( obj )                  (( gboolean ) GPOINTER_TO_UINT( fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_MATCHCASE )))
#define fma_object_get_mimetypes( obj )                  (( GSList * ) fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_MIMETYPES ))
#define fma_object_get_all_mimetypes( obj )              (( gboolean ) GPOINTER_TO_UINT( fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_MIMETYPES_IS_ALL )))
#define fma_object_get_folders( obj )                    (( GSList * ) fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_FOLDERS ))
#define fma_object_get_schemes( obj )                    (( GSList * ) fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_SCHEMES ))
#define fma_object_get_only_show_in( obj )               (( GSList * ) fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_ONLY_SHOW ))
#define fma_object_get_not_show_in( obj )                (( GSList * ) fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_NOT_SHOW ))
#define fma_object_get_try_exec( obj )                   (( gchar * ) fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_TRY_EXEC ))
#define fma_object_get_show_if_registered( obj )         (( gchar * ) fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_SHOW_IF_REGISTERED ))
#define fma_object_get_show_if_true( obj )               (( gchar * ) fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_SHOW_IF_TRUE ))
#define fma_object_get_show_if_running( obj )            (( gchar * ) fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_SHOW_IF_RUNNING ))
#define fma_object_get_selection_count( obj )            (( gchar * ) fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_SELECTION_COUNT ))
#define fma_object_get_capabilities( obj )               (( GSList * ) fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_CAPABILITITES ))

#define fma_object_set_basenames( obj, bnames )          fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_BASENAMES, ( const void * )( bnames ))
#define fma_object_set_matchcase( obj, match )           fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_MATCHCASE, ( const void * ) GUINT_TO_POINTER( match ))
#define fma_object_set_mimetypes( obj, types )           fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_MIMETYPES, ( const void * )( types ))
#define fma_object_set_all_mimetypes( obj, all )         fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_MIMETYPES_IS_ALL, ( const void * ) GUINT_TO_POINTER( all ))
#define fma_object_set_folders( obj, folders )           fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_FOLDERS, ( const void * )( folders ))
#define fma_object_replace_folder( obj, old, new )       fma_icontext_replace_folder( FMA_ICONTEXT( obj ), ( const gchar * )( old ), ( const gchar * )( new ))
#define fma_object_set_scheme( obj, scheme, add )        fma_icontext_set_scheme( FMA_ICONTEXT( obj ), ( const gchar * )( scheme ), ( add ))
#define fma_object_set_schemes( obj, schemes )           fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_SCHEMES, ( const void * )( schemes ))
#define fma_object_set_only_show_in( obj, list )         fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_ONLY_SHOW, ( const void * )( list ))
#define fma_object_set_only_desktop( obj, desktop, add ) fma_icontext_set_only_desktop( FMA_ICONTEXT( obj ), ( const gchar * )( desktop ), ( add ))
#define fma_object_set_not_show_in( obj, list )          fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_NOT_SHOW, ( const void * )( list ))
#define fma_object_set_not_desktop( obj, desktop, add )  fma_icontext_set_not_desktop( FMA_ICONTEXT( obj ), ( const gchar * )( desktop ), ( add ))
#define fma_object_set_try_exec( obj, exec )             fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_TRY_EXEC, ( const void * )( exec ))
#define fma_object_set_show_if_registered( obj, name )   fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_SHOW_IF_REGISTERED, ( const void * )( name ))
#define fma_object_set_show_if_true( obj, exec )         fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_SHOW_IF_TRUE, ( const void * )( exec ))
#define fma_object_set_show_if_running( obj, name )      fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_SHOW_IF_RUNNING, ( const void * )( name ))
#define fma_object_set_selection_count( obj, cond )      fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_SELECTION_COUNT, ( const void * )( cond ))
#define fma_object_set_capabilities( obj, cap )          fma_ifactory_object_set_from_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_CAPABILITITES, ( const void * )( cap ))

#ifdef NA_ENABLE_DEPRECATED
#define fma_object_set_modified( obj, modified )         fma_iduplicable_set_modified( FMA_IDUPLICABLE( obj ), ( modified ))
#endif

G_END_DECLS

#endif /* __FILE_MANAGER_ACTIONS_API_OBJECT_API_H__ */
