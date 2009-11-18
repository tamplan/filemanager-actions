dnl define three distinct log domains, respectively for common code,
dnl plugin and NACT user interface - log handlers will be disabled
dnl when not in development mode

# serial 2 define NA-runtime log domain

AC_DEFUN([NA_LOG_DOMAINS],[
	AC_SUBST([NA_LOGDOMAIN_API],[NA-api])
	AC_DEFINE_UNQUOTED([NA_LOGDOMAIN_API],["NA-api"],[Log domain of API library])

	AC_SUBST([NA_LOGDOMAIN_NACT],[NA-nact])
	AC_DEFINE_UNQUOTED([NA_LOGDOMAIN_NACT],["NA-nact"],[Log domain of NACT user interface])

	AC_SUBST([NA_LOGDOMAIN_PLUGIN],[NA-plugin])
	AC_DEFINE_UNQUOTED([NA_LOGDOMAIN_PLUGIN],["NA-plugin"],[Log domain of Nautilus plugin])

	AC_SUBST([NA_LOGDOMAIN_PRIVATE],[NA-private])
	AC_DEFINE_UNQUOTED([NA_LOGDOMAIN_PRIVATE],["NA-private"],[Log domain of private library])

	AC_SUBST([NA_LOGDOMAIN_RUNTIME],[NA-runtime])
	AC_DEFINE_UNQUOTED([NA_LOGDOMAIN_RUNTIME],["NA-runtime"],[Log domain of runtime library])

	AC_SUBST([NA_LOGDOMAIN_TEST],[NA-test])
	AC_DEFINE_UNQUOTED([NA_LOGDOMAIN_TEST],["NA-test"],[Log domain of test programs])

	AC_SUBST([NA_LOGDOMAIN_UTILS],[NA-utils])
	AC_DEFINE_UNQUOTED([NA_LOGDOMAIN_UTILS],["NA-utils"],[Log domain of utilities])
])
