/*
 * Nautilus Actions
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
 *   and many others (see AUTHORS)
 *
 * pwi 2009-05-16 fix compilation warnings
 */

#include <config.h>
#include <stdio.h>
#include <glib.h>
#include <glib/gi18n.h>
#include "nautilus-actions-tools-utils.h"

gboolean nautilus_actions_file_set_contents (const gchar* filename,
															const gchar* contents,
															gssize length,
															GError** error)
{
	gboolean success = FALSE;

#ifdef HAVE_GLIB_2_8
	success = g_file_set_contents (filename, contents, length, error);
#else
	FILE* fp;

	if ((fp = fopen (filename, "wt")) != NULL)
	{
		if (fprintf (fp, contents) == length)
		{
			success = TRUE;
		}
		else
		{
			(*error) = g_error_new (G_FILE_ERROR, G_FILE_ERROR_FAILED, _("Can't write data in file %s\n"), filename);
		}
		fclose (fp);
	}
	else
	{
		(*error) = g_error_new (G_FILE_ERROR, G_FILE_ERROR_FAILED, _("Can't open file %s for writing\n"), filename);
	}
#endif

	return success;
}
