AC_INIT([ncd], 0.1, [rene@renesp.com.br])
AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror foreign])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([z],[deflate],,AC_MSG_ERROR([ERROR: can't find zlib!]))
ZLIB_LIBS='-lz'
AC_SUBST(ZLIB_LIBS)

AC_CHECK_LIB([bz2],[BZ2_bzCompress],,AC_MSG_ERROR([ERROR: can't find bzlib!]))
ZLIB_LIBS='-lbz2'
AC_SUBST(BZLIB_LIBS)

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

