/* Nautilus Actions configuration tool
 * Copyright (C) 2005 The GNOME Foundation
 *
 * Authors:
 *  Frederic Ruaudel (grumz@grumz.net)
 *	 Rodrigo Moya (rodrigo@gnome-db.org)
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

#ifndef _NAUTILUS_ACTIONS_CONFIG_XML_H_
#define _NAUTILUS_ACTIONS_CONFIG_XML_H_

#include <glib/glist.h>
#include <glib-object.h>
#include <glib.h>
#include <gconf/gconf-client.h>
#include "nautilus-actions-config.h"
#include <glib/gi18n.h>

#ifdef N_
#undef N_
#endif
#define N_(String) String

G_BEGIN_DECLS

// Error data
#define NAUTILUS_ACTIONS_XML_ERROR g_quark_from_string ("nautilus_actions_config_xml")

typedef enum
{
   NAUTILUS_ACTIONS_XML_ERROR_FAILED
} NautilusActionsXmlError;


#define NAUTILUS_ACTIONS_TYPE_CONFIG_XML            (nautilus_actions_config_xml_get_type())
#define NAUTILUS_ACTIONS_CONFIG_XML(obj)            (G_TYPE_CHECK_INSTANCE_CAST (obj, NAUTILUS_ACTIONS_TYPE_CONFIG_XML, NautilusActionsConfigXml))
#define NAUTILUS_ACTIONS_CONFIG_XML_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST (klass, NAUTILUS_ACTIONS_TYPE_CONFIG_XML, NautilusActionsConfigXmlClass))
#define NAUTILUS_ACTIONS_IS_CONFIG_XML(obj)         (G_TYPE_CHECK_INSTANCE_TYPE(obj, NAUTILUS_ACTIONS_TYPE_CONFIG_XML))
#define NAUTILUS_ACTIONS_IS_CONFIG_XML_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), NAUTILUS_ACTIONS_TYPE_CONFIG_XML))

typedef struct _NautilusActionsConfigXml NautilusActionsConfigXml;
typedef struct _NautilusActionsConfigXmlClass NautilusActionsConfigXmlClass;

struct _NautilusActionsConfigXml {
	NautilusActionsConfig parent;

	/* Private data, don't access */
};

struct _NautilusActionsConfigXmlClass {
	NautilusActionsConfigClass parent_class;
};

GType                        nautilus_actions_config_xml_get_type (void);
NautilusActionsConfigXml       *nautilus_actions_config_xml_get (void);
gboolean nautilus_actions_config_xml_parse_file (NautilusActionsConfigXml* config, const gchar* filename, GError** error);
void nautilus_actions_config_xml_load_list (NautilusActionsConfigXml* config);

G_END_DECLS

#endif
