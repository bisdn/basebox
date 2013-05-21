AC_INIT(vmcore, 0.1.0, rofl@bisdn.de, vmcore)
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_MACRO_DIR([m4])
AC_GNU_SOURCE

m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

AC_PROG_INSTALL
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LD
AUTOCONF_ENV="SHELL=/bin/sh"

LT_INIT

AC_ENABLE_STATIC

# Check for debug mode - MUST BE THE FIRST CHECK
AC_MSG_CHECKING(whether to enable debug mode)
debug_default="no"
AC_ARG_ENABLE(debug,
	AS_HELP_STRING([--enable-debug], [turn on debug mode [default=no]])
		, , enable_debug=$debug_default)
if test "$enable_debug" = "yes"; then
	CFLAGS="-g -O0"
	CXXFLAGS="-g -O0 -fno-inline"
	AC_DEFINE(DEBUG)
	AC_MSG_RESULT(yes)
else
	CXXFLAGS="-O3 -fomit-frame-pointer"
	CFLAGS="-O3 --compiler-options -fno-strict-aliasing --compiler-options -fno-inline --compiler-bindir=/usr/bin/g++-4.3"
	AC_DEFINE(NDEBUG)
	AC_MSG_RESULT(no)
fi

# Check for profiling mode
AC_MSG_CHECKING(whether to enable profiling mode)
profile_default="no"
AC_ARG_ENABLE(profile,
	AS_HELP_STRING([--enable-profile], [turn on profile mode [default=no]])
		, , enable_profile=$profile_default)
if test "$enable_profile" = "yes"; then
	CFLAGS="$( echo $CFLAGS | sed s/-fomit-frame-pointer//g )"
	CXXFLAGS="$( echo $CXXFLAGS | sed s/-fomit-frame-pointer//g )"
	CXXFLAGS="$CXXFLAGS -pg"
	LDFLAGS="$LDFLAGS -pg"
	AC_MSG_RESULT(yes)
else
	AC_MSG_RESULT(no)
fi

#Check libs used
#AC_CHECK_LIB(pthread, pthread_create)
#AC_CHECK_LIB(rt, shm_open)
#AC_CHECK_LIB(cli, ???)

# AC_CONFIG_HEADERS([include/config.h])
AC_CONFIG_FILES([
	Makefile
	src/Makefile
	src/vmcore/Makefile
	test/Makefile
	test/vmcore/Makefile
	])
AC_OUTPUT
