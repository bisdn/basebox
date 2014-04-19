# Set application version based on the git version

#Default
ROFCORED_VERSION="Unknown (no GIT repository detected)"

AC_CHECK_PROG(ff_git,git,yes,no)

if test $ff_git = no
then
	AC_MSG_RESULT([git not found!])
else

	AC_MSG_CHECKING([the build number(version)])
	
	if test -d $srcdir/.git ; then
		#Try to retrieve the build number
		_ROFCORED_GIT_BUILD=`git log -1 --pretty=%H`
		_ROFCORED_GIT_BRANCH=`git rev-parse --abbrev-ref HEAD`
		ROFCORED_VERSION=`git describe --abbrev=0`
		_ROFCORED_GIT_DESCRIBE=`git describe --abbrev=40`

		AC_DEFINE_UNQUOTED([ROFCORED_BUILD],["$_ROFCORED_GIT_BUILD"])
		AC_DEFINE_UNQUOTED([ROFCORED_BRANCH],["$_ROFCORED_GIT_BRANCH"])
		AC_DEFINE_UNQUOTED([ROFCORED_DESCRIBE],["$_ROFCORED_GIT_DESCRIBE"])

	fi

	AC_MSG_RESULT($ROFCORED_VERSION)
fi

AC_DEFINE_UNQUOTED([ROFCORED_VERSION],["$ROFCORED_VERSION"])
