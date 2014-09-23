AC_INIT(vmcore, 0.1.0, rofl@bisdn.de, vmcore)
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



PKG_CHECK_MODULES([LIBNL3], libnl-3.0 >= 3.1 libnl-route-3.0 >= 3.1, [have_libnl3=yes], [have_libnl3=no])
if (test "${have_libnl3}" = "yes"); then
        CPPFLAGS+="$LIBNL3_CFLAGS"
        LIBS+="$LIBNL3_LIBS"
fi

PKG_CHECK_MODULES([ROFL], rofl >= 0.3.0, [have_rofl=yes], [have_rofl=no])
if (test "${have_rofl}" = "yes"); then
        CPPFLAGS+="$ROFL_CFLAGS"
        LIBS+="$ROFL_LIBS"
fi


# libpcap include and libs
AC_ARG_WITH([pcap-include],
  [AS_HELP_STRING([--with-pcap-include],
    [location of pcap header, defaults to /usr/include])],
  [PCAP_CFLAGS="-I$withval"],
  [PCAP_CFLAGS='-I/usr/include'])
AC_SUBST([PCAP_CFLAGS])

AC_ARG_WITH([pcap-lib],
  [AS_HELP_STRING([--with-pcap-lib], [location of pcap library])],
  [PCAP_LIBS="-L$withval -lpcap"],
  [PCAP_LIBS='-lpcap'])
AC_SUBST([PCAP_LIBS])



PKG_CHECK_MODULES([LIBCONFIGXX], [libconfig++ >= 1.4], [have_libconfig=yes], AC_MSG_ERROR([libconfig++ 1.4 or newer not found.]))
if (test "${have_libconfig}" = "yes"); then
        CPPFLAGS+="$LIBCONFIGXX_CFLAGS"
        LIBS+="$LIBCONFIGXX_LIBS"
fi


# version
m4_include([config/versioning.m4])


#Check libs used
#AC_CHECK_LIB(pthread, pthread_create)
#AC_CHECK_LIB(rt, shm_open)
#AC_CHECK_LIB(cli, ???)

# AC_CONFIG_HEADERS([include/config.h])
AC_CONFIG_FILES([
	Makefile
	src/Makefile
	src/dhcpv6snoop/Makefile
	src/baseboxd/Makefile
	src/ethboxd/Makefile
	src/librofnetlink/Makefile
	src/librofipcore/Makefile
	src/librofethcore/Makefile
	src/librofethcore/librofrstp/Makefile
	src/librofgtpcore/Makefile
	src/librofgrecore/Makefile
	
	test/Makefile
	test/ipcore/Makefile
	])
AC_OUTPUT
