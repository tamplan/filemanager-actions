# Nautilus-Actions
# A Nautilus extension which offers configurable context menu actions.
#
# Copyright (C) 2005 The GNOME Foundation
# Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
# Copyright (C) 2009-2014 Pierre Wieser and others (see AUTHORS)
#
# Nautilus-Actions is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# Nautilus-Actions is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Nautilus-Actions; see the file COPYING. If not, see
# <http://www.gnu.org/licenses/>.
#
# Authors:
#   Frederic Ruaudel <grumz@grumz.net>
#   Rodrigo Moya <rodrigo@gnome-db.org>
#   Pierre Wieser <pwieser@trychlos.org>
#   ... and many others (see AUTHORS)

dnl define three distinct log domains, respectively for common code,
dnl plugin and NACT user interface - log handlers will be disabled
dnl when not in development mode

# serial 2 define NA-runtime log domain

AC_DEFUN([NA_LOG_DOMAINS],[
	AC_SUBST([NA_LOGDOMAIN_IO_DESKTOP],[NA-io-desktop])
	AC_DEFINE_UNQUOTED([NA_LOGDOMAIN_IO_DESKTOP],["NA-io-desktop"],[Log domain of desktop I/O Provider])

	AC_SUBST([NA_LOGDOMAIN_IO_GCONF],[NA-io-gconf])
	AC_DEFINE_UNQUOTED([NA_LOGDOMAIN_IO_GCONF],["NA-io-gconf"],[Log domain of GConf I/O Provider])

	AC_SUBST([NA_LOGDOMAIN_IO_XML],[NA-io-xml])
	AC_DEFINE_UNQUOTED([NA_LOGDOMAIN_IO_XML],["NA-io-xml"],[Log domain of XML I/O])

	AC_SUBST([NA_LOGDOMAIN_CORE],[NA-core])
	AC_DEFINE_UNQUOTED([NA_LOGDOMAIN_CORE],["NA-core"],[Log domain of core library])

	AC_SUBST([NA_LOGDOMAIN_NACT],[NA-nact])
	AC_DEFINE_UNQUOTED([NA_LOGDOMAIN_NACT],["NA-nact"],[Log domain of NACT user interface])

	AC_SUBST([NA_LOGDOMAIN_PLUGIN_MENU],[NA-plugin-menu])
	AC_DEFINE_UNQUOTED([NA_LOGDOMAIN_PLUGIN_MENU],["NA-plugin-menu"],[Log domain of Nautilus Menu plugin])

	AC_SUBST([NA_LOGDOMAIN_PLUGIN_TRACKER],[NA-plugin-tracker])
	AC_DEFINE_UNQUOTED([NA_LOGDOMAIN_PLUGIN_TRACKER],["NA-plugin-tracker"],[Log domain of Nautilus Tracker plugin])

	AC_SUBST([NA_LOGDOMAIN_TEST],[NA-test])
	AC_DEFINE_UNQUOTED([NA_LOGDOMAIN_TEST],["NA-test"],[Log domain of test programs])

	AC_SUBST([NA_LOGDOMAIN_UTILS],[NA-utils])
	AC_DEFINE_UNQUOTED([NA_LOGDOMAIN_UTILS],["NA-utils"],[Log domain of utilities])
])
