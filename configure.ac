AC_INIT([ncd], 0.1, [rene@renesp.com.br])
AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror foreign])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_ARG_WITH([zlib],
    AS_HELP_STRING([--without-zlib], [Disable zlib support]),[with_zlib=no],[with_zlib=check])

AS_IF([test "x$with_zlib" != "xno"],
      [AC_CHECK_LIB([z],[deflate],[have_zlib=yes],AC_MSG_ERROR([ERROR: can't find zlib!]))],
      [have_zlib=no])

AS_IF([test "x$have_zlib" = "xyes"],
      [ZLIB_LIBS='-lz'
	   AC_SUBST(ZLIB_LIBS)
	   AC_DEFINE([HAVE_ZLIB],[1],[Define to enable zlib compressor support])],)

AC_ARG_WITH([bzlib],
    AS_HELP_STRING([--without-bzlib], [Disable bzlib support]))

AS_IF([test "x$with_bzlib" != "xno"],
      [AC_CHECK_LIB([bz2],[BZ2_bzCompress],[have_bzlib=yes],AC_MSG_ERROR([ERROR: can't find bzlib!]))],
      [have_zlib=no])

AS_IF([test "x$have_bzlib" = "xyes"],
      [BZLIB_LIBS='-lbz2'
	   AC_SUBST(BZLIB_LIBS)]
	   AC_DEFINE([HAVE_BZLIB],[1],[Define to enable bzlib compressor support]),)

AC_SEARCH_LIBS(pthread_create,[pthread],,AC_MSG_ERROR([ERROR: can't find pthreds!]))

# Checks for header files.
AC_CHECK_HEADERS([getopt.h libgen.h])

# Checks for typedefs, structures, and compiler characteristics.
# Checks for library functions.

# Output files.
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_FILES([
	Makefile
	src/Makefile
])

AC_OUTPUT

