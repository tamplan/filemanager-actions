dnl define three distinct log domains, respectively for common code,
dnl plugin and NACT user interface - log handlers will be disabled
dnl when not in development mode

# serial 1 creation

AC_DEFUN([NA_LOG_DOMAINS],[
	AC_SUBST([NA_LOGDOMAIN_COMMON],[NA-common])
	AC_DEFINE_UNQUOTED([NA_LOGDOMAIN_COMMON],["NA-common"],[Log domain of common code])

	AC_SUBST([NA_LOGDOMAIN_PLUGIN],[NA-plugin])
	AC_DEFINE_UNQUOTED([NA_LOGDOMAIN_PLUGIN],["NA-plugin"],[Log domain of Nautilus plugin])

	AC_SUBST([NA_LOGDOMAIN_NACT],[NA-nact])
	AC_DEFINE_UNQUOTED([NA_LOGDOMAIN_NACT],["NA-nact"],[Log domain of NACT user interface])

	AC_SUBST([NA_LOGDOMAIN_TEST],[NA-test])
	AC_DEFINE_UNQUOTED([NA_LOGDOMAIN_TEST],["NA-test"],[Log domain of test programs])

	AC_SUBST([NA_LOGDOMAIN_UTILS],[NA-utils])
	AC_DEFINE_UNQUOTED([NA_LOGDOMAIN_UTILS],["NA-utils"],[Log domain of utilities])
])
