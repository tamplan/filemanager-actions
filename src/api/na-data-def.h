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

#ifndef __NAUTILUS_ACTIONS_API_NA_FACTORY_DATA_DEF_H__
#define __NAUTILUS_ACTIONS_API_NA_FACTORY_DATA_DEF_H__

/**
 * SECTION: na_ifactory_object
 * @short_description: Data Factory Definitions.
 * @include: nautilus-actions/na-factory-object-str.h
 */

#include <glib-object.h>

G_BEGIN_DECLS

/**
 * The structure which fully describes an elementary factory data
 * Each #NAIFactoryObject item definition may include several groups of
 * this structure
 */
typedef struct {
	gchar    *name;						/* both the id and the canonical name
										 * used when getting/setting properties
										 * must be globally unique
										 * must also be an invariant as it is known from plugin extensions */

	gboolean  readable;					/* whether the data should be read on unserialization ops.
										 * if FALSE, then no attempt will be made to read it
										 * and the data will has to be set dynamically
										 * when a data has been written once (see below), and unless
										 * special cases (see e.g. type), it should remain readable
										 * even if it has becomen obsolete (for backward compatibility) */

	gboolean  writable;					/* whether the data is to be written on serialization ops.
										 * if FALSE, then no attempt will be made to write it
										 * mainly set to FALSE to dynamically set variables and
										 * obsoleted ones */

	gboolean  has_property;				/* whether a property should be set for this variable ?
										 * set to FALSE for obsolete variables */

	gchar    *short_label;				/* short descriptive name
										 * used in GParamSpec and in schemas */

	gchar    *long_label;				/* long, if not complete, description
										 * used in GParamSpec and in schemas */

	guint     type;						/* the elementary NAFD_TYPE_xxx data type */

	gchar    *default_value;			/* the default to assign when creating a new object
										 * this default is also displayed in command-line help
										 * of nautilus-actions-new utility */

	gboolean  copyable;					/* whether this data should be automatically copied when
										 * we are duplicating an object to another
										 * in all cases, the implementation is always triggered
										 * by the copy() interface method */

	gboolean  comparable;				/* whether this data should be compared when we
										 * are testing two objects for equality */

	gboolean  mandatory;				/* whether this data must be not null and not empty
										 * when we are testing for validity of an object */

	gboolean  localizable;				/* whether this is a localizable data
										 * when serializing or exporting */

	gchar    *gconf_entry;				/* same entry is also used for GConf-based XML docs */
}
	NADataDef;

/**
 * The structure which fully describes a logical group of data
 * Each #NAIFactoryObject item definition is built from a list of
 * these groups
 */
typedef struct {
	gchar     *group;					/* defined in na-ifactory-object-data.h */
	NADataDef *def;
}
	NADataGroup;

G_END_DECLS

#endif /* __NAUTILUS_ACTIONS_API_NA_FACTORY_DATA_DEF_H__ */
