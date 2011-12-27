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

#ifndef __CORE_NA_IOPTIONS_LIST_H__
#define __CORE_NA_IOPTIONS_LIST_H__

/*
 * SECTION: ioptions_list
 * @title: NAIOptionsList
 * @short_description: The OptionsList Interface v 1
 * @include: core/na-ioptions-list.h
 *
 * The #NAIOptionsList interface is to be used when we have one option to choose
 * among several. The interface takes care of:
 * <itemizedlist>
 *   <listitem>
 *     <para>
 *       displaying the items, either as a radio button group or as a tree view,
 *       inside of a parent container;
 *     </para>
 *     <para>
 *       maybe displaying a tooltip and/or a pixbuf for each option;
 *     </para>
 *     <para>
 *       maybe adding an 'Ask me' option;
 *     </para>
 *     <para>
 *       setting a default option, identified by its identifier, which can
 *       be anything resolvable to a #gpointer value;
 *     </para>
 *     <para>
 *       returning the selected option as a #gpointer value;
 *     </para>
 *     <para>
 *       freeing the allocated resources at end.
 *     </para>
 *   </listitem>
 * </itemizedlist>
 *
 * In order this interface to work, each option has to be seen as a #GObject
 * -derived object, which itself should implement the #NAIOption companion
 * interface.
 *
 * More, the instance which would want to implement this #NAIOptionsList
 * interface should also implement one of #NAIOptionsRadioButton or
 * #NAIOptionsTreeView interfaces.
 *
 * <refsect2>
 *  <title>Versions historic</title>
 *  <table>
 *    <title>Historic of the versions of the #NAIOptionsList interface</title>
 *    <tgroup rowsep="1" colsep="1" align="center" cols="3">
 *      <colspec colname="na-version" />
 *      <colspec colname="api-version" />
 *      <colspec colname="current" />
 *      <thead>
 *        <row>
 *          <entry>&prodname; version</entry>
 *          <entry>#NAIOptionsList interface version</entry>
 *          <entry></entry>
 *        </row>
 *      </thead>
 *      <tbody>
 *        <row>
 *          <entry>since 3.2</entry>
 *          <entry>1</entry>
 *          <entry>current version</entry>
 *        </row>
 *      </tbody>
 *    </tgroup>
 *  </table>
 * </refsect2>
 */

#include <gtk/gtk.h>

#include "na-ioption.h"

G_BEGIN_DECLS

#define NA_IOPTIONS_LIST_TYPE               ( na_ioptions_list_get_type())
#define NA_IOPTIONS_LIST( i )               ( G_TYPE_CHECK_INSTANCE_CAST( i, NA_IOPTIONS_LIST_TYPE, NAIOptionsList ))
#define NA_IS_IOPTIONS_LIST( i )            ( G_TYPE_CHECK_INSTANCE_TYPE( i, NA_IOPTIONS_LIST_TYPE ))
#define NA_IOPTIONS_LIST_GET_INTERFACE( i ) ( G_TYPE_INSTANCE_GET_INTERFACE(( i ), NA_IOPTIONS_LIST_TYPE, NAIOptionsListInterface ))

typedef struct _NAIOptionsList                      NAIOptionsList;
typedef struct _NAIOptionsListInterfacePrivate      NAIOptionsListInterfacePrivate;
typedef struct _NAIOptionsListImportFromUriParms    NAIOptionsListImportFromUriParms;
typedef struct _NAIOptionsListManageImportModeParms NAIOptionsListManageImportModeParms;

/*
 * NAIOptionsListInterface:
 * @get_version:     returns the version of this interface that the
 *                   instance implements.
 *
 * This defines the interface that a #NAIOptionsList implementation should provide.
 */
typedef struct {
	/*< private >*/
	GTypeInterface                  parent;
	NAIOptionsListInterfacePrivate *private;

	/*< public >*/
	/*
	 * get_version:
	 * @instance: the #NAIOptionsList instance of the implementation.
	 *
	 * This method is supposed to let know to any caller which version of this
	 * interface the implementation provides. This may be useful when this
	 * interface will itself be upgraded.
	 *
	 * If this method is not provided by the implementation, one should suppose
	 * that the implemented version is at last the version 1.
	 *
	 * Returns: the version of this interface provided by the implementation.
	 *
	 * Since: 3.2
	 */
	guint       ( *get_version ) ( const NAIOptionsList *instance );

	/*
	 * get_options:
	 * @instance: the #NAIOptionsList instance of the implementation.
	 *
	 * This method may be called at more or less early stage of the build
	 * of the display, either rather early when displaying a radio button
	 * group, or later in the case of a tree view.
	 *
	 * Returns: a #GList list of #NAIOptions object instances.
	 *
	 * Since: 3.2
	 */
	GList *     ( *get_options ) ( const NAIOptionsList *instance );

	/*
	 * free_options:
	 * @instance: the #NAIOptionsList instance of the implementation.
	 * @options: a #GList of #NAIoption objects as returned by get_options() method.
	 *
	 * Release the resources allocated to the @options list.
	 *
	 * Note that the interface defaults to just g_object_unref() each
	 * instance of the @options list, the g_list_free() the list itself.
	 * So if your implementation may satisfy itself with this default, you
	 * just do not need to implement the method.
	 *
	 * Since: 3.2
	 */
	void        ( *free_options )( const NAIOptionsList *instance, GList *options );

	/*
	 * get_ask_option:
	 * @instance: the #NAIOptionsList instance of the implementation.
	 *
	 * Ask the implementation to provide a #NAIOption which defines the
	 * 'Ask me' option.
	 *
	 * Returns: a #NAIOption which defines the 'Ask me' option.
	 *
	 * Since: 3.2
	 */
	NAIOption * ( *get_ask_option ) ( const NAIOptionsList *instance );

	/*
	 * free_ask_option:
	 * @instance: the #NAIOptionsList instance of the implementation.
	 * @ask_option: the #NAIoption to be released.
	 *
	 * Release the resources allocated to the @ask_option instance.
	 *
	 * Note that the interface defaults to just g_object_unref() the object.
	 * So if your implementation may satisfy itself with this default, you just
	 * do not need to implement the method.
	 *
	 * Since: 3.2
	 */
	void        ( *free_ask_option )( const NAIOptionsList *instance, NAIOption *ask_option );
}
	NAIOptionsListInterface;

GType na_ioptions_list_get_type( void );

void  na_ioptions_list_display_init( NAIOptionsList *instance, GtkWidget *parent, gboolean with_ask, gboolean editable, gboolean sensitive );
void  na_ioptions_list_set_default ( NAIOptionsList *instance, NAIOption *default_option );

G_END_DECLS

#endif /* __CORE_NA_IOPTIONS_LIST_H__ */
