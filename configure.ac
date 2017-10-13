AC_INIT(baseboxd, m4_esyscmd_s([./version.sh]), [https://github.com/bisdn/basebox], baseboxd, [https://github.com/bisdn/basebox])

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([-Wall -Werror foreign])

m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PROG_INSTALL
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LD
AC_PROG_SED
PKG_PROG_PKG_CONFIG

AC_CHECK_FUNCS([bzero])
AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([select])
AC_CHECK_FUNCS([socket])
AC_CHECK_FUNCS([strerror])
AC_CHECK_HEADERS([fcntl.h])
AC_CHECK_HEADERS([sys/ioctl.h])
AC_CHECK_HEADERS([sys/socket.h])
AC_CHECK_HEADERS([sys/time.h])
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_FUNC_ERROR_AT_LINE
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

LT_INIT

AC_ENABLE_STATIC

BASEBOX_VERSION

CFLAGS+=" -Wall -Werror"
CXXFLAGS+=" -Wall -Werror"

# Check for debug mode - MUST BE THE FIRST CHECK
AC_MSG_CHECKING(whether to enable debug mode)
debug_default="no"
AC_ARG_ENABLE(debug,
	AS_HELP_STRING([--enable-debug], [turn on debug mode [default=no]])
		, , enable_debug=$debug_default)
if test "$enable_debug" = "yes"; then
	CFLAGS="$CFLAGS -g -O0"
	CXXFLAGS="$CXXFLAGS -g -O0 -fno-inline"
	AC_DEFINE(DEBUG)
	AC_MSG_RESULT(yes)
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

# check for libraries
PKG_CHECK_MODULES([LIBNL3], libnl-3.0 >= 3.2.29 libnl-route-3.0 >= 3.2.29,
  [ CPPFLAGS="$CPPFLAGS $LIBNL3_CFLAGS"
	LIBS="$LIBS $LIBNL3_LIBS"],
  [ AC_MSG_ERROR([minimum version of libnl-3 is 3.2.29]) ])

PKG_CHECK_MODULES([ROFL], rofl_common >= 0.12.1,
  [ CPPFLAGS="$CPPFLAGS $ROFL_CFLAGS"
	LIBS="$LIBS $ROFL_LIBS" ],
  [ AC_MSG_ERROR([minimum version of rofl_common is 0.12.1]) ])

PKG_CHECK_MODULES([ROFL_OFDPA], rofl_ofdpa >= 0.9,
  [ CPPFLAGS="$CPPFLAGS $ROFL_OFDPA_CFLAGS"
	LIBS="$LIBS $ROFL_OFDPA_LIBS" ],
  [ AC_MSG_ERROR([minimum version of rofl_ofdpa is 0.9]) ])

AC_CHECK_LIB([gflags], [getenv], [GFLAGS_LIBS="-lgflags"],
  AC_MSG_ERROR([libgflags not found]) )

PKG_CHECK_MODULES([GLOG], libglog >= 0.3.3,
  [ CPPFLAGS="$CPPFLAGS $GLOG_CFLAGS"
	LIBS="$LIBS $GLOG_LIBS $GFLAGS_LIBS" ],
  [ AC_MSG_ERROR([minimum version of glog is 0.3.3]) ])

# todo: add http://www.gnu.org/software/autoconf-archive/ax_cxx_compile_stdcxx_11.html
CXXFLAGS="$CXXFLAGS -std=c++11"

# AC_CONFIG_HEADERS([include/config.h])
AC_CONFIG_FILES([
	Makefile
	src/Makefile
	src/baseboxd/Makefile
	src/roflibs/Makefile
	src/roflibs/netlink/Makefile
	src/roflibs/of-dpa/Makefile
	pkg/rhel/baseboxd.spec
	])
AC_OUTPUT
