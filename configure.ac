AC_INIT(baseboxd, m4_esyscmd_s([./version.sh]), baseboxd-devel@roflibs.org, baseboxd, http://www.roflibs.org)

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

LT_INIT

AC_ENABLE_STATIC

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
PKG_CHECK_MODULES([LIBNL3], libnl-3.0 >= 3.2.26 libnl-route-3.0 >= 3.2.26,
  [ CPPFLAGS+="$LIBNL3_CFLAGS"
	LIBS+="$LIBNL3_LIBS"
	# the latest changes of libnl are not yet release, hence another check here
    AC_SEARCH_LIBS([rtnl_link_bridge_get_port_vlan], [libnl-route], [], 
      [AC_MSG_ERROR([libnl-route is too old, currently you have to use https://github.com/thom311/libnl master])]) ],
  [ AC_MSG_ERROR([minimum version of libnl-3 is 3.2.26]) ])

PKG_CHECK_MODULES([ROFL], rofl_common >= 0.10.0,
  [ CPPFLAGS+="$ROFL_CFLAGS"
	LIBS+="$ROFL_LIBS" ],
  [ AC_MSG_ERROR([minimum version of rofl_common is 0.10.0]) ])

PKG_CHECK_MODULES([LIBCONFIGXX], [libconfig++ >= 1.4],
  [ CPPFLAGS+="$LIBCONFIGXX_CFLAGS"
	LIBS+="$LIBCONFIGXX_LIBS"],
  [ AC_MSG_ERROR([libconfig++ 1.4 or newer not found.])] )
  
PKG_CHECK_MODULES([ROFL_OFDPA], rofl_ofdpa >= 0.1,
  [ CPPFLAGS+="$ROFL_OFDPA_CFLAGS"
	LIBS+="$ROFL_OFDPA_LIBS" ],
  [ AC_MSG_ERROR([minimum version of rofl_ofdpa is 0.1]) ])

# todo: add http://www.gnu.org/software/autoconf-archive/ax_cxx_compile_stdcxx_11.html
CXXFLAGS="$CXXFLAGS -std=c++11"

# version
#m4_include([config/versioning.m4])

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
	src/roflibs/of-dpa/Makefile
	pkg/rhel/basebox.spec
	])
AC_OUTPUT
