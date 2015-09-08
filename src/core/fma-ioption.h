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

#ifndef __CORE_FMA_IOPTION_H__
#define __CORE_FMA_IOPTION_H__

/*
 * SECTION: ioptions
 * @title: FMAIOption
 * @short_description: The Option Interface v 1
 * @include: core/fma-ioption.h
 *
 * The #FMAIOption interface is to be implemented by #GObject -derived object which
 * are part of a #FMAIOptionsList interface.
 *
 * <refsect2>
 *  <title>Versions historic</title>
 *  <table>
 *    <title>Historic of the versions of the #FMAIOption interface</title>
 *    <tgroup rowsep="1" colsep="1" align="center" cols="3">
 *      <colspec colname="na-version" />
 *      <colspec colname="api-version" />
 *      <colspec colname="current" />
 *      <thead>
 *        <row>
 *          <entry>&prodname; version</entry>
 *          <entry>#FMAIOption interface version</entry>
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

#include "gdk-pixbuf/gdk-pixbuf.h"

G_BEGIN_DECLS

#define FMA_TYPE_IOPTION                      ( fma_ioption_get_type())
#define FMA_IOPTION( instance )               ( G_TYPE_CHECK_INSTANCE_CAST( instance, FMA_TYPE_IOPTION, FMAIOption ))
#define FMA_IS_IOPTION( instance )            ( G_TYPE_CHECK_INSTANCE_TYPE( instance, FMA_TYPE_IOPTION ))
#define FMA_IOPTION_GET_INTERFACE( instance ) ( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), FMA_TYPE_IOPTION, FMAIOptionInterface ))

typedef struct _FMAIOption                      FMAIOption;
typedef struct _FMAIOptionInterfacePrivate      FMAIOptionInterfacePrivate;
typedef struct _FMAIOptionImportFromUriParms    FMAIOptionImportFromUriParms;
typedef struct _FMAIOptionManageImportModeParms FMAIOptionManageImportModeParms;

/*
 * FMAIOptionInterface:
 * @get_version:     returns the version of this interface that the
 *                   instance implements.
 * @get_id:          returns the string identifier of the option.
 * @get_label:       returns the label of the option.
 * @get_description: returns the description of the option.
 * @get_pixbuf:      returns the image associated to the option.
 *
 * This defines the interface that a #FMAIOption implementation should provide.
 */
typedef struct {
	/*< private >*/
	GTypeInterface              parent;
	FMAIOptionInterfacePrivate *private;

	/*< public >*/
	/*
	 * get_version:
	 * @instance: the #FMAIOption instance of the implementation.
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
	guint       ( *get_version )    ( const FMAIOption *instance );

	/*
	 * get_id:
	 * @instance: the #FMAIOption instance of the implementation.
	 *
	 * Returns: the string identifier of the option, as a newly allocated string
	 * which should be g_free() by the caller.
	 *
	 * Since: 3.2
	 */
	gchar *     ( *get_id )         ( const FMAIOption *instance );

	/*
	 * get_label:
	 * @instance: the #FMAIOption instance of the implementation.
	 *
	 * Returns: the label of the option, as a newly allocated string
	 * which should be g_free() by the caller.
	 *
	 * Since: 3.2
	 */
	gchar *     ( *get_label )      ( const FMAIOption *instance );

	/*
	 * get_description:
	 * @instance: the #FMAIOption instance of the implementation.
	 *
	 * Returns: the description of the option, as a newly allocated string
	 * which should be g_free() by the caller.
	 *
	 * Since: 3.2
	 */
	gchar *     ( *get_description )( const FMAIOption *instance );

	/*
	 * get_pixbuf:
	 * @instance: the #FMAIOption instance of the implementation.
	 *
	 * Returns: the image assocated to the option, as a newly allocated string
	 * which should be g_object_unref() by the caller.
	 *
	 * Since: 3.2
	 */
	GdkPixbuf * ( *get_pixbuf )     ( const FMAIOption *instance );
}
	FMAIOptionInterface;

GType      fma_ioption_get_type       ( void );

gchar     *fma_ioption_get_id         ( const FMAIOption *option );
gchar     *fma_ioption_get_label      ( const FMAIOption *option );
gchar     *fma_ioption_get_description( const FMAIOption *option );
GdkPixbuf *fma_ioption_get_pixbuf     ( const FMAIOption *option );

G_END_DECLS

#endif /* __CORE_FMA_IOPTION_H__ */
