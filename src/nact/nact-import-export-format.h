/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009 Pierre Wieser and others (see AUTHORS)
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

#ifndef __NACT_IMPORT_EXPORT_FORMAT_H__
#define __NACT_IMPORT_EXPORT_FORMAT_H__

G_BEGIN_DECLS

/* import/export formats
 *
 * FORMAT_GCONFSCHEMAFILE_V1: a schema with owner, short and long
 * descriptions ; each action has its own schema addressed by the uuid
 * (historical format up to v1.10.x serie)
 *
 * FORMAT_GCONFSCHEMAFILE_V2: the lightest schema still compatible
 * with gconftool-2 --install-schema-file (no owner, no short nor long
 * descriptions) - introduced in v 1.11
 *
 * FORMAT_GCONFENTRY: not a schema, but a dump of the GConf entry
 * introduced in v 1.11
 */
enum {
	FORMAT_GCONFSCHEMAFILE_V1 = 1,
	FORMAT_GCONFSCHEMAFILE_V2,
	FORMAT_GCONFENTRY
};

G_END_DECLS

#endif /* __NACT_IMPORT_EXPORT_FORMAT_H__ */
