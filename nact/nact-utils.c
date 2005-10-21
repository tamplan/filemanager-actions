/* Nautilus Actions configuration tool
 * Copyright (C) 2005 The GNOME Foundation
 *
 * Authors:
 *  Frederic Ruaudel (grumz@grumz.net)
 *
 * This Program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This Program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this Library; see the file COPYING.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include "nact-utils.h"

#define GLADE_FILE GLADEDIR "/nautilus-actions-config.glade"

static GHashTable* get_glade_object_hashtable ()
{
	static GHashTable* glade_object_hash = NULL;

	if (glade_object_hash == NULL)
	{
		glade_object_hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
	}

	return glade_object_hash;
}

GladeXML* nact_get_glade_xml_object (const gchar* root_widget)
{
	GHashTable* glade_object_hash = get_glade_object_hashtable ();
	GladeXML* retv = NULL;

	retv = (GladeXML*)g_hash_table_lookup (glade_object_hash, root_widget);
	if (retv == NULL)
	{
		retv = glade_xml_new (GLADE_FILE, root_widget, NULL);

		g_hash_table_insert (glade_object_hash, g_strdup (root_widget), retv);
	}

	return GLADE_XML (g_object_ref (retv));
}

GtkWidget* nact_get_glade_widget_from (const gchar* widget_name, const gchar* root_widget)
{
	GladeXML* xml = nact_get_glade_xml_object (root_widget);
	return glade_xml_get_widget (xml, widget_name);
}

GtkWidget* nact_get_glade_widget (const gchar* widget_name)
{
	return nact_get_glade_widget_from (widget_name, GLADE_MAIN_WIDGET);
}

GList* nact_get_glade_widget_prefix_from (const gchar* widget_name, const gchar* root_widget)
{
	GladeXML* xml = nact_get_glade_xml_object (root_widget);
	return glade_xml_get_widget_prefix (xml, widget_name);
}

GList* nact_get_glade_widget_prefix (const gchar* widget_name)
{
	return nact_get_glade_widget_prefix_from (widget_name, GLADE_MAIN_WIDGET);
}

void nact_destroy_glade_objects ()
{
	GHashTable* glade_object_hash = get_glade_object_hashtable ();
	g_hash_table_destroy (glade_object_hash);
}
// vim:ts=3:sw=3:tw=1024:cin
