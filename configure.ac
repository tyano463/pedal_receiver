AC_PREREQ([2.69])
AC_INIT([PedalReceiver], [1.0], [mushoku@msn.com])
AM_INIT_AUTOMAKE([foreign -Wno-portability no-dependencies])

AC_CANONICAL_HOST

AC_PROG_CC

if [[ "x${target}" != "x" ]] ; then
    CFLAGS="$CFLAGS --sysroot=$STAGING_DIR"
    CXXFLAGS="$CXXFLAGS --sysroot=$STAGING_DIR"
    LDFLAGS="$LDFLAGS --sysroot=$STAGING_DIR"
fi

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
