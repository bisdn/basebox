AC_INIT(baseboxd, m4_esyscmd_s(cat VERSION), baseboxd-devel@roflibs.org, baseboxd, http://www.roflibs.org)

AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_MACRO_DIR([m4])
AC_GNU_SOURCE

m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

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
	CFLAGS="-O3 -fomit-frame-pointer"
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



PKG_CHECK_MODULES([LIBNL3], libnl-3.0 >= 3.1 libnl-route-3.0 >= 3.1, [have_libnl3=yes], [have_libnl3=no])
if (test "${have_libnl3}" = "yes"); then
        CPPFLAGS+="$LIBNL3_CFLAGS"
        LIBS+="$LIBNL3_LIBS"
fi

PKG_CHECK_MODULES([ROFL], rofl_common >= 0.6.0, [have_rofl=yes], [have_rofl=no])
if (test "${have_rofl}" = "yes"); then
        CPPFLAGS+="$ROFL_CFLAGS"
        LIBS+="$ROFL_LIBS"
fi



PKG_CHECK_MODULES([LIBCONFIGXX], [libconfig++ >= 1.4], [have_libconfig=yes], AC_MSG_ERROR([libconfig++ 1.4 or newer not found.]))
if (test "${have_libconfig}" = "yes"); then
        CPPFLAGS+="$LIBCONFIGXX_CFLAGS"
        LIBS+="$LIBCONFIGXX_LIBS"
fi


# version
m4_include([config/versioning.m4])

#AM_PATH_PYTHON([2.7])

#	src/roflibs/python/Makefile
#	src/roflibs/flowcore/Makefile
#	src/roflibs/8021core/Makefile
#	src/roflibs/ethcore/Makefile
#	src/roflibs/ethcore/rstp/Makefile
#	src/roflibs/ipcore/Makefile
#	src/roflibs/gtpcore/Makefile
#	src/roflibs/grecore/Makefile


# AC_CONFIG_HEADERS([include/config.h])
AC_CONFIG_FILES([
	Makefile
	src/Makefile
	src/baseboxd/Makefile
	src/roflibs/Makefile
	src/roflibs/netlink/Makefile
	
	test/Makefile
	test/ipcore/Makefile
	])
AC_OUTPUT
