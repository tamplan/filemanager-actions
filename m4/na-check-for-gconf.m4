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

# serial 1 creation

dnl pwi 2011-02-14
dnl this is a copy of the original AM_GCONF_SOURCE_2 which is just a bit hacked
dnl in order to define the conditionals event when we want disabled GConf
dnl syntax: NA_GCONF_SOURCE_2([have_gconf])
dnl
dnl AM_GCONF_SOURCE_2
dnl Defines GCONF_SCHEMA_CONFIG_SOURCE which is where you should install schemas
dnl  (i.e. pass to gconftool-2)
dnl Defines GCONF_SCHEMA_FILE_DIR which is a filesystem directory where
dnl  you should install foo.schemas files

AC_DEFUN([NA_GCONF_SOURCE_2],
[
	if test "$1" = "yes"; then
		if test "x$GCONF_SCHEMA_INSTALL_SOURCE" = "x"; then
			GCONF_SCHEMA_CONFIG_SOURCE=`gconftool-2 --get-default-source`
		else
			GCONF_SCHEMA_CONFIG_SOURCE=$GCONF_SCHEMA_INSTALL_SOURCE
		fi
	fi

  AC_ARG_WITH([gconf-source],
              AC_HELP_STRING([--with-gconf-source=sourceaddress],
                             [Config database for installing schema files.]),
              [GCONF_SCHEMA_CONFIG_SOURCE="$withval"],)

  AC_SUBST(GCONF_SCHEMA_CONFIG_SOURCE)

	if test "$1" = "yes"; then
		AC_MSG_RESULT([Using config source $GCONF_SCHEMA_CONFIG_SOURCE for schema installation])
	fi

  if test "x$GCONF_SCHEMA_FILE_DIR" = "x"; then
    GCONF_SCHEMA_FILE_DIR='$(sysconfdir)/gconf/schemas'
  fi

  AC_ARG_WITH([gconf-schema-file-dir],
              AC_HELP_STRING([--with-gconf-schema-file-dir=dir],
                             [Directory for installing schema files.]),
              [GCONF_SCHEMA_FILE_DIR="$withval"],)

  AC_SUBST(GCONF_SCHEMA_FILE_DIR)

	if test "$1" = "yes"; then
		AC_MSG_RESULT([Using $GCONF_SCHEMA_FILE_DIR as install directory for schema files])
	fi

  AC_ARG_ENABLE(schemas-install,
        AC_HELP_STRING([--disable-schemas-install],
                       [Disable the schemas installation]),
     [case ${enableval} in
       yes|no) ;;
       *) AC_MSG_ERROR([bad value ${enableval} for --enable-schemas-install]) ;;
      esac])

	if test "x${enable_schemas_install}" = "xno" -o "$1" != "yes"; then
		msg_schemas_install="disabled"; else
		msg_schemas_install="enabled in ${GCONF_SCHEMA_FILE_DIR}"
	fi
      
  AM_CONDITIONAL([GCONF_SCHEMAS_INSTALL], [test "$enable_schemas_install" != no -a "$1" = "yes"])
])

dnl let the user choose whether to compile with GConf enabled
dnl --enable-gconf
dnl
dnl defaults to automatically enable GConf if it is present on the compiling
dnl system, or disable it if it is absent
dnl if --enable-gconf is specified, then GConf subsystem must be present

AC_DEFUN([NA_CHECK_FOR_GCONF],[
	AC_REQUIRE([_AC_NA_ARG_GCONF])dnl
	AC_REQUIRE([_AC_NA_CHECK_GCONF])dnl
])

AC_DEFUN([_AC_NA_ARG_GCONF],[
	AC_ARG_ENABLE(
		[gconf],
		AC_HELP_STRING(
			[--enable-gconf],
			[whether to enable GConf subsystem @<:@auto@:>@]
		),
	[enable_gconf=$enableval],
	[enable_gconf="auto"]
	)
])

AC_DEFUN([_AC_NA_CHECK_GCONF],[
	AC_MSG_CHECKING([whether GConf is enabled])
	AC_MSG_RESULT([${enable_gconf}])

	if test "${enable_gconf}" = "auto"; then
		AC_PATH_PROG([GCONFTOOL],[gconftool-2],[no])
		if test "${GCONFTOOL}" = "no"; then
			enable_gconf="no"
		else
			enable_gconf="yes"
		fi
	else
		if test "${enable_gconf}" = "yes"; then
			AC_PATH_PROG([GCONFTOOL],[gconftool-2],[no])
			if test "${GCONFTOOL}" = "no"; then
				AC_MSG_ERROR([gconftool-2: program not found])
			fi
		fi
	fi
	
	if test "${enable_gconf}" = "yes"; then
		AC_SUBST([AM_CPPFLAGS],["${AM_CPPFLAGS} -DHAVE_GCONF"])
		AC_DEFINE_UNQUOTED([HAVE_GCONF],[1],[Whether we compile against the GConf library])
	fi

	NA_GCONF_SOURCE_2(["${enable_gconf}"])
	AM_CONDITIONAL([HAVE_GCONF], [test "${enable_gconf}" = "yes"])
])
