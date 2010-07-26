/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu actions.
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

#ifndef __NACT_MATCH_LIST_H__
#define __NACT_MATCH_LIST_H__

/**
 * SECTION: nact_match_list
 * @short_description: Implementation of a list match/does not match.
 * @include: nact/nact-match-list.h
 */

#include "base-window.h"

G_BEGIN_DECLS

typedef GSList * ( *pget_filters )( void * );
typedef void     ( *pset_filters )( void *, GSList * );
typedef void     ( *pon_add_cb )  ( void *, BaseWindow * );

enum {
	MATCH_LIST_MUST_MATCH_ONE_OF = 1,
	MATCH_LIST_MUST_MATCH_ALL_OF,
};

typedef struct {
	BaseWindow      *window;
	guint            tab_id;
	GtkTreeView     *listview;
	GtkWidget       *addbutton;
	GtkWidget       *removebutton;
	pget_filters     pget;
	pset_filters     pset;
	pon_add_cb       pon_add;
	guint            match_header;
	gchar           *item_header;
	gboolean         editable_filter;
	/* dynamic data */
	gboolean         editable_item;
	guint            sort_column;
	guint            sort_order;
}
	MatchListStr;

void    nact_match_list_create_model        ( BaseWindow *window, const gchar *tab_name,
			guint tab_id,
			GtkWidget *listview, GtkWidget *addbutton, GtkWidget *removebutton,
			pget_filters pget, pset_filters pset, pon_add_cb pon_add,
			guint match_header,
			const gchar *item_header,
			gboolean editable_filter );

void    nact_match_list_init_view           ( BaseWindow *window, const gchar *tab_name );

void    nact_match_list_on_selection_changed( BaseWindow *window, const gchar *tab_name,
			guint count );

void    nact_match_list_insert_row          ( BaseWindow *window, const gchar *tab_name,
			const gchar *filter, gboolean match, gboolean not_match );

GSList *nact_match_list_get_rows            ( BaseWindow *window, const gchar *tab_name );

void    nact_match_list_dispose             ( BaseWindow *window, const gchar *tab_name );

G_END_DECLS

#endif /* __NACT_MATCH_LIST_H__ */
