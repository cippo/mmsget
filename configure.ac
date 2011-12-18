AC_INIT([mmsget], [0.1], [sivertb@stud.ntnu.no])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
AC_PROG_CC
AC_PROG_CC_C99
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/Makefile])
AC_CHECK_LIB(pthread, pthread_mutex_init)
PKG_CHECK_MODULES([LIBMMS], [libmms])
AC_OUTPUT
