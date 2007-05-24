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

#ifndef _NACT_PROFILE_EDITOR_H_
#define _NACT_PROFILE_EDITOR_H_

#include <libnautilus-actions/nautilus-actions-config.h>
#include <libnautilus-actions/nautilus-actions-config-gconf-writer.h>

gboolean nact_profile_editor_new_profile (NautilusActionsConfigAction* action);
gboolean nact_profile_editor_edit_profile (NautilusActionsConfigAction *action, gchar* profile_name, NautilusActionsConfigActionProfile* action_profile);

#endif
