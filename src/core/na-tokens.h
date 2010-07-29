/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu modules.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010 Pierre Wieser and others (see AUTHORS)
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

#ifndef __CORE_NA_TOKENS_H__
#define __CORE_NA_TOKENS_H__

/**
 * SECTION: na_tokens
 * @short_description: #NATokens class definition.
 * @include: core/na-tokens.h
 *
 * The #NATokens class manages the tokens which are to be replaced with
 * elements of the current selection at runtime.
 *
 * Note that until v2.30, tokens were parsed against selection list only
 * when an item was selected in the Nautilus context menu (i.e. at
 * execution time).
 * Starting with unstable v2.99 (stable v3.0), this same parsing may occur
 * for each displayed label (as new specs accept tokens in labels) - we so
 * factorize this parsing one time for each new selection in the Nautilus
 * plugin, attaching the result to each item in the context menu.
 *
 * Adding a parameter requires updating of :
 * - src/core/na-tokens.c::na_tokens_is_singular_exec()
 * - src/core/na-tokens.c::na_tokens_parse_parameters()
 * - nautilus-actions/nact/nact-icommand-tab.c:parse_parameters()
 * - src/nact/nautilus-actions-config-tool.ui:LegendDialog
 */

#include <api/na-object-profile.h>

G_BEGIN_DECLS

#define NA_TOKENS_TYPE					( na_tokens_get_type())
#define NA_TOKENS( object )				( G_TYPE_CHECK_INSTANCE_CAST( object, NA_TOKENS_TYPE, NATokens ))
#define NA_TOKENS_CLASS( klass )		( G_TYPE_CHECK_CLASS_CAST( klass, NA_TOKENS_TYPE, NATokensClass ))
#define NA_IS_TOKENS( object )			( G_TYPE_CHECK_INSTANCE_TYPE( object, NA_TOKENS_TYPE ))
#define NA_IS_TOKENS_CLASS( klass )		( G_TYPE_CHECK_CLASS_TYPE(( klass ), NA_TOKENS_TYPE ))
#define NA_TOKENS_GET_CLASS( object )	( G_TYPE_INSTANCE_GET_CLASS(( object ), NA_TOKENS_TYPE, NATokensClass ))

typedef struct NATokensPrivate      NATokensPrivate;

typedef struct {
	GObject          parent;
	NATokensPrivate *private;
}
	NATokens;

typedef struct NATokensClassPrivate NATokensClassPrivate;

typedef struct {
	GObjectClass          parent;
	NATokensClassPrivate *private;
}
	NATokensClass;

GType     na_tokens_get_type( void );

NATokens *na_tokens_new_from_selection( GList *selection );

gchar    *na_tokens_parse_parameters( const NATokens *tokens, const gchar *string, gboolean utf8 );

void      na_tokens_execute_action  ( const NATokens *tokens, const NAObjectProfile *profile );

G_END_DECLS

#endif /* __CORE_NA_TOKENS_H__ */
