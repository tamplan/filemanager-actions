/*
 * FileManager-Actions
 * A file-manager extension which offers configurable context menu modules.
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

#ifndef __CORE_FMA_TOKENS_H__
#define __CORE_FMA_TOKENS_H__

/* @title: FMATokens
 * @short_description: The #FMATokens Class Definition
 * @include: core/fma-tokens.h
 *
 * The #FMATokens class manages the tokens which are to be replaced with
 * elements of the current selection at runtime.
 *
 * Note that until v2.30, tokens were parsed against selection list only
 * when an item was selected in the file manager context menu (i.e. at
 * execution time).
 * Starting with unstable v2.99 (stable v3.0), this same parsing may occur
 * for each displayed label (as new specs accept tokens in labels) - we so
 * factorize this parsing one time for each new selection in the menu
 * plugin, attaching the result to each item in the context menu.
 *
 * Adding a parameter requires updating of:
 * - doc/nact/C/figures/nact-legend.png screenshot
 * - doc/nact/C/nact-execution.xml "Multiple execution" paragraph
 * - src/core/fma-tokens.c::is_singular_exec() function
 * - src/core/fma-tokens.c::parse_singular() function
 * - src/nact/filemanager-actions-config-tool.ui:LegendDialog labels
 * - src/core/fma-object-profile-factory.c:FMAFO_DATA_PARAMETERS comment
 *
 * Valid parameters are :
 *
 * %b: (first) basename
 * %B: space-separated list of basenames
 * %c: count of selected items
 * %d: (first) base directory
 * %D: space-separated list of base directory of each selected item
 * %f: (first) file name
 * %F: space-separated list of selected file names
 * %h: hostname of the (first) URI
 * %m: (first) mimetype
 * %M: space-separated list of mimetypes
 * %n: username of the (first) URI
 * %o: no-op operator which forces a singular form of execution
 * %O: no-op operator which forces a plural form of execution
 * %p: port number of the (first) URI
 * %s: scheme of the (first) URI
 * %u: (first) URI
 * %U: space-separated list of selected URIs
 * %w: (first) basename without the extension
 * %W: space-separated list of basenames without their extension
 * %x: (first) extension
 * %X: space-separated list of extensions
 * %%: the « % » character
 */

#include <api/fma-object-profile.h>

G_BEGIN_DECLS

#define FMA_TYPE_TOKENS                ( fma_tokens_get_type())
#define FMA_TOKENS( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, FMA_TYPE_TOKENS, FMATokens ))
#define FMA_TOKENS_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, FMA_TYPE_TOKENS, FMATokensClass ))
#define FMA_IS_TOKENS( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, FMA_TYPE_TOKENS ))
#define FMA_IS_TOKENS_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), FMA_TYPE_TOKENS ))
#define FMA_TOKENS_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), FMA_TYPE_TOKENS, FMATokensClass ))

typedef struct _FMATokensPrivate       FMATokensPrivate;

typedef struct {
	/*< private >*/
	GObject           parent;
	FMATokensPrivate *private;
}
	FMATokens;

typedef struct _FMATokensClassPrivate  FMATokensClassPrivate;

typedef struct {
	/*< private >*/
	GObjectClass           parent;
	FMATokensClassPrivate *private;
}
	FMATokensClass;

GType      fma_tokens_get_type            ( void );

FMATokens *fma_tokens_new_for_example     ( void );
FMATokens *fma_tokens_new_from_selection  ( GList *selection );

gchar     *fma_tokens_parse_for_display   ( const FMATokens *tokens, const gchar *string, gboolean utf8 );
void       fma_tokens_execute_action      ( const FMATokens *tokens, const FMAObjectProfile *profile );

gchar     *fma_tokens_command_for_terminal( const gchar *pattern, const gchar *command );

G_END_DECLS

#endif /* __CORE_FMA_TOKENS_H__ */
