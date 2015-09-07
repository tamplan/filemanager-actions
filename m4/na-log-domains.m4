# FileManager-Actions
# A file-manager extension which offers configurable context menu actions.
#
# Copyright (C) 2005 The GNOME Foundation
# Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
# Copyright (C) 2009-2015 Pierre Wieser and others (see AUTHORS)
#
# FileManager-Actions is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# FileManager-Actions is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with FileManager-Actions; see the file COPYING. If not, see
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
	AC_SUBST([NA_LOGDOMAIN_IO_DESKTOP],[NA])
	AC_DEFINE_UNQUOTED([NA_LOGDOMAIN_IO_DESKTOP],["NA"],[Log domain of desktop I/O Provider])

	AC_SUBST([NA_LOGDOMAIN_IO_GCONF],[NA])
	AC_DEFINE_UNQUOTED([NA_LOGDOMAIN_IO_GCONF],["NA"],[Log domain of GConf I/O Provider])

	AC_SUBST([NA_LOGDOMAIN_IO_XML],[NA])
	AC_DEFINE_UNQUOTED([NA_LOGDOMAIN_IO_XML],["NA"],[Log domain of XML I/O])

	AC_SUBST([NA_LOGDOMAIN_CORE],[NA])
	AC_DEFINE_UNQUOTED([NA_LOGDOMAIN_CORE],["NA"],[Log domain of core library])

	AC_SUBST([NA_LOGDOMAIN_NACT],[NA])
	AC_DEFINE_UNQUOTED([NA_LOGDOMAIN_NACT],["NA"],[Log domain of NACT user interface])

	AC_SUBST([NA_LOGDOMAIN_PLUGIN_MENU],[NA])
	AC_DEFINE_UNQUOTED([NA_LOGDOMAIN_PLUGIN_MENU],["NA"],[Log domain of Nautilus Menu plugin])

	AC_SUBST([NA_LOGDOMAIN_PLUGIN_TRACKER],[NA])
	AC_DEFINE_UNQUOTED([NA_LOGDOMAIN_PLUGIN_TRACKER],["NA"],[Log domain of Nautilus Tracker plugin])

	AC_SUBST([NA_LOGDOMAIN_TEST],[NA])
	AC_DEFINE_UNQUOTED([NA_LOGDOMAIN_TEST],["NA"],[Log domain of test programs])

	AC_SUBST([NA_LOGDOMAIN_UTILS],[NA])
	AC_DEFINE_UNQUOTED([NA_LOGDOMAIN_UTILS],["NA"],[Log domain of utilities])
])
