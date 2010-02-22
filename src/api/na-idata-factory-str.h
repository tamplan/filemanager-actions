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

#ifndef __NAUTILUS_ACTIONS_API_NA_IDATA_FACTORY_STR_H__
#define __NAUTILUS_ACTIONS_API_NA_IDATA_FACTORY_STR_H__

/**
 * SECTION: na_idata_factory
 * @short_description: Data Factory Definitions.
 * @include: nautilus-actions/na-data-factory-str.h
 */

#include <glib-object.h>

G_BEGIN_DECLS

/**
 * Elementary data types
 * Each object data item must be typed as one of these
 * IFactoryProvider implementations should provide a primitive for reading
 * (resp. writing) a value for each of these elementary data types.
 *
 * IMPORTANT NOTE
 * Please note that this enumeration may  be compiled in by extensions.
 * They must so remain fixed, unless you want see strange effects (e.g.
 * an extension has been compiled with NADF_TYPE_STRING = 2, while you
 * have inserted another element, making it to 3 !) - or you know what
 * you are doing...
 */

enum {
	NADF_TYPE_STRING = 1,				/* an ASCII string */

	NADF_TYPE_LOCALE_STRING,			/* a localized UTF-8 string */

	NADF_TYPE_BOOLEAN,					/* a boolean
										 * can be initialized with "true" or "false" (case insensitive) */

	NADF_TYPE_STRING_LIST,				/* a list of ASCII strings */

	NADF_TYPE_POINTER,					/* a ( void * ) pointer
										 * should be initialized to NULL */

	NADF_TYPE_UINT,						/* an unsigned integer */
};

/* attach here a xml document root with the corresponding node for the data
 */
typedef struct {
	gchar *doc_id;
	gchar *key;
}
	NadfDocKey;

/**
 * The structure which fully describe an elementary data
 * Each #NAIDataFactory item definition may include several groups of
 * this structure
 */
typedef struct {
	guint     id;						/* the id of the object data item
										 * must only be unique inside of the given group */

	gchar    *name;						/* canonical name, used when getting/setting properties */

	gboolean  serializable;				/* whether the data is serializable
										 * if FALSE, then no attempt will be made to read/write it
										 * and the data will must be set dynamically */

	gchar    *short_label;				/* short descriptive name, used in GParamSpec */

	gchar    *long_label;				/* long, if not complete, description, used in GParamSpec */

	guint     type;						/* the elementary NADF_TYPE_xxx data type */

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

	void   ( *free )( void * );			/* a pointer to a function to free the element data
										 * a default function is provided for main elementary
										 * data types:
										 * - STRING and LOCALE_STRING: g_free
										 * - STRING_LIST: na_core_utils_slist_free
										 * - BOOLEAN, UINT, POINTER: none
										 *
										 * This may be used mainly when POINTER type is used
										 * to cast e.g. a GList of items */
}
	NadfIdType;

/**
 * The structure which fully describe a logical group of data
 * Each #NAIDataFactory item may definition may be built from a list of
 * these groups
 */
typedef struct {
	guint       idgroup;				/* cf. na-idata-factory-enum.h */
	NadfIdType *iddef;
}
	NadfIdGroup;

G_END_DECLS

#endif /* __NAUTILUS_ACTIONS_API_NA_IDATA_FACTORY_STR_H__ */
