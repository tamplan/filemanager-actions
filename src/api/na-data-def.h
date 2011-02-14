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

#ifndef __NAUTILUS_ACTIONS_API_NA_FACTORY_DATA_DEF_H__
#define __NAUTILUS_ACTIONS_API_NA_FACTORY_DATA_DEF_H__

/**
 * SECTION: data-def
 * @title: NADataDef, NADataGroup
 * @short_description: The Data Factory Structure Definitions
 * @include: nautilus-actions/na-data-def.h
 *
 * #NADataDef and #NADataGroup are structures which handle the list of
 * elementary datas for each and every #NAObjectItem which happens to
 * implement the #NAIFactoryObject interface.
 */

#include <glib.h>

G_BEGIN_DECLS

/**
 * NADataDef:
 * @name:             both the id and the canonical name.
 *                    Used when getting/setting properties.
 *                    Is defined in na-ifactory-object-data.h and must be globally unique.
 *                    Must be an invariant as it is known from plugin extensions.
 * @readable:         whether the data should be read on unserialization operations.
 *                    If FALSE, then no attempt will be made to read it
 *                    and the data will have to be set dynamically.
 *                    When a data has been written once (see below), and unless
 *                    special cases (see e.g. type), it should remain readable
 *                    even if it has becomen obsolete (for backward compatibility).
 * @writable:         whether the data is to be written on serialization operations.
 *                    If FALSE, then no attempt will be made to write it.
 *                    Mainly set to FALSE for dynamically set variables and
 *                    obsoleted ones.
 * @has_property:     whether a property should be set for this variable ?
 *                    Set to FALSE for obsolete variables.
 * @short_label:      short localizable descriptive name.
 *                    Used in GParamSpec and in schemas.
 * @long_label:       long, if not complete, localizable description.
 *                    Used in GParamSpec and in schemas?
 * @type:             the elementary NA_DATA_TYPE_xxx data type.
 * @default_value:    the default to assign when creating a new object.
 *                    This default is also displayed in command-line help
 *                    of nautilus-actions-new utility.
 * @write_if_default: write this value even if it is the default value ?
 *                    Should default to FALSE.
 * @copyable:         whether this data should be automatically copied when
 *                    we are duplicating an object to another ?
 *                    In all cases, the implementation is always triggered
 *                    by the copy() interface method.
 * @comparable:       whether this data should be compared when we
 *                    are testing two objects for equality.
 * @mandatory:        whether this data must be not null and not empty
 *                    when we are testing for validity of an object.
 * @localizable:      whether this is a localizable data when serializing or exporting.
 * @gconf_entry:      same entry is also used for GConf-based XML docs.
 * @desktop_entry:    entry in .desktop files.
 * @option_short:     the short version of a command-line parameter in nautilus-actions-new,
 *                    or 0.
 * @option_long:      the long version of the same command-line parameter in nautilus-actions-new,
 *                    or NULL.
 * @option_flags:     #GOptionFlags for the command-line parameter, or 0.
 * @option_arg:       the type of the option, or 0.
 * @option_label:     the localizable description for the variable in nautilus-actions-new.
 *                    Defaults to @short_label if NULL.
 * @option_arg_label: the localizable description for the argument.
 *
 * This structure fully describes an elementary factory data.
 * Each #NAIFactoryObject item definition may include several groups of
 * this structure.
 */
typedef struct {
	gchar     *name;
	gboolean   readable;
	gboolean   writable;
	gboolean   has_property;
	gchar     *short_label;
	gchar     *long_label;
	guint      type;
	gchar     *default_value;
	gboolean   write_if_default;
	gboolean   copyable;
	gboolean   comparable;
	gboolean   mandatory;
	gboolean   localizable;
	gchar     *gconf_entry;
	gchar     *desktop_entry;
	gchar      option_short;
	gchar     *option_long;
	gint       option_flags;
	GOptionArg option_arg;
	gchar     *option_label;
	gchar     *option_arg_label;
}
	NADataDef;

/**
 * NADataGroup:
 * @group: the name of the group, as defined in na-ifactory-object-data.h.
 * @def: the list of the corresponding data structures.
 *
 * This structure fully describes a logical group of data.
 * Each #NAIFactoryObject item definition is built from a list of
 * these groups.
 */
typedef struct {
	gchar     *group;
	NADataDef *def;
}
	NADataGroup;

const NADataDef *na_data_def_get_data_def( const NADataGroup *group, const gchar *group_name, const gchar *name );

G_END_DECLS

#endif /* __NAUTILUS_ACTIONS_API_NA_FACTORY_DATA_DEF_H__ */
