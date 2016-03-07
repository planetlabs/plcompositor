AC_INIT(plcompositor)
AC_CONFIG_MACRO_DIR([m4/])
AC_PREREQ([2.69])
AC_CONFIG_SRCDIR([src/])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Checks for libraries.
AX_LIB_GDAL
AC_CHECK_LIB([wjreader], [_WJROpenDocument], [], [AC_MSG_ERROR(
    [Unable to find the WJElement libs (hint: use LDFLAGS=-L/path/to/wjelement/lib)])]
)

# Checks for header files.
AC_CHECK_HEADER([wjreader.h], [], [AC_MSG_ERROR(
    [Unable to find wjreader.h (hint: use CPPFLAGS="-I/path/to/wjelement/include/")])]
)
AC_CHECK_HEADER([wjelement.h], [], [AC_MSG_ERROR(
    [Unable to find wjelement.h (hint: use CPPFLAGS="-I/path/to/wjelement/include/")])]
)

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL

# Checks for library functions.
AC_CHECK_FUNCS([floor memset strerror])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 tests/Makefile])
AC_OUTPUT
