# Process this file with autoconf to produce a configure script.
# Configure template for chardet library
#
# $Id: configure.ac,v 1.4 2009/02/23 13:56:47 oops Exp $

#
AC_PREREQ(2.59)
AC_INIT([chardet], [1.0.1], [http://oops.org])
AC_CONFIG_SRCDIR([src/nsUniversalDetector.h])
AC_CONFIG_HEADER([chardet-config.h])
AC_PREFIX_DEFAULT([/usr/local])

AC_SUBST(SONAME_INFO)
AC_SUBST(SONAME_VERSION)
AC_SUBST(SONAME_MAJOR)
AC_SUBST(SONAME_MINOR)
AC_SUBST(SONAME_REVISION)

SONAME_MAJOR=1
SONAME_MINOR=0
SONAME_REVISION=0
SONAME_VERSION=$SONAME_MAJOR.$SONAME_MINOR.$SONAME_REVISION

SONAME_VALUE=$(($SONAME_MAJOR + $SONAME_MINOR))
SONAME_INFO="$SONAME_VALUE:$SONAME_REVISION:$SONAME_MINOR"

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_AWK
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

AC_PATH_PROG(MAKE, make)
if test ! -f "$ac_cv_path_MAKE"; then
	AC_MSG_ERROR(Can't not found make)
fi

AC_PATH_PROG(STRIP, strip)
if test ! -f "$ac_cv_path_STRIP"; then
	AC_MSG_ERROR(Can't not found strip)
fi

AC_PATH_PROG(RM, rm)
if test ! -f "$ac_cv_path_RM"; then
	AC_MSG_ERROR(Can't not found rm)
fi

AC_PATH_PROG(MKDIR, mkdir)
if test ! -f "$ac_cv_path_MKDIR"; then
	AC_MSG_ERROR(Can't not found mkdir)
fi

AC_PATH_PROG(PERL, perl)
if test ! -f "$ac_cv_path_PERL"; then
	AC_MSG_ERROR(Can't not found perl)
fi

# Checks for header files.
AC_HEADER_STDC

AC_CHECK_HEADERS([stdlib.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_HEADER_TIME

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_STAT

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
