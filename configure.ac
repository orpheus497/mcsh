dnl Autoconf script for mcsh (Modern C Shell) — a consolidated, modernised
dnl fusion of tcsh and etcsh.  To rebuild the `configure' script from this,
dnl execute the command
dnl     autoreconf -fi
dnl in the directory containing this script.
dnl
dnl Derived from tcsh's configure.ac, originally written by
dnl Kaveh Ghazi (ghazi@caip.rutgers.edu) 5/11/96.

dnl
dnl UPDATE THIS SECTION FOR RELEASE
dnl
AC_DEFUN([MCSH_VERSION], [0.1.0])
AC_DEFUN([MCSH_DATE],    [2026-04-20])
dnl Upstream tcsh baseline that mcsh was consolidated from.
AC_DEFUN([TCSH_VERSION], [6.24.13])
AC_DEFUN([TCSH_DATE],    [2024-06-12])

AC_PREREQ([2.69])dnl Minimum Autoconf version; prefer 2.72+ for modern macro support.
AC_INIT([mcsh],MCSH_VERSION,[https://github.com/orpheus497/mcsh/issues],[mcsh],[https://github.com/orpheus497/mcsh])

AC_SUBST(PACKAGE_DATE,        MCSH_DATE)
AC_SUBST(PACKAGE_ORIGIN,      [mcsh])
AC_SUBST(PACKAGE_MAILLIST,    [https://github.com/orpheus497/mcsh/issues])
AC_SUBST(TCSH_BASELINE_VERS,  TCSH_VERSION)
AC_SUBST(TCSH_BASELINE_DATE,  TCSH_DATE)
AC_DEFINE_UNQUOTED([TCSH_BASELINE_VERSION],
    ["TCSH_VERSION"],
    [Upstream tcsh version that mcsh was consolidated from.])

package_year="${PACKAGE_DATE%%-*}"
package_month="${PACKAGE_DATE#*-}"
package_month="${package_month%-*}"
package_day="${PACKAGE_DATE##*-}"
package_day="${package_day#0}"

AS_CASE([${package_month}],
[01], [package_month_name=January],
[02], [package_month_name=February],
[03], [package_month_name=March],
[04], [package_month_name=April],
[05], [package_month_name=May],
[06], [package_month_name=June],
[07], [package_month_name=July],
[08], [package_month_name=August],
[09], [package_month_name=September],
[10], [package_month_name=October],
[11], [package_month_name=November],
[12], [package_month_name=December])

PACKAGE_ENGLISH_DATE="${package_month_name} ${package_day}, ${package_year}"
AC_SUBST(PACKAGE_ENGLISH_DATE)

package_changelog_date_format='+%a, %d %b %Y %T %z'
AC_SUBST(PACKAGE_CHANGELOG_DATE)

PACKAGE_REV="${PACKAGE_VERSION%%.*}"
AC_SUBST(PACKAGE_REV)

PACKAGE_VERS="${PACKAGE_VERSION#*.}"
PACKAGE_VERS="${PACKAGE_VERS%.*}"
AC_SUBST(PACKAGE_VERS)

PACKAGE_PATCHLEVEL="${PACKAGE_VERSION##*.}"
PACKAGE_PATCHLEVEL="$(printf '%d' "$PACKAGE_PATCHLEVEL" 2>/dev/null || echo "$PACKAGE_PATCHLEVEL")"
AC_SUBST(PACKAGE_PATCHLEVEL)

RELEASE_TAG="$(echo "MCSH${PACKAGE_VERSION}" | tr . _)"
AC_SUBST(RELEASE_TAG)

AC_CONFIG_AUX_DIR([acaux])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([tc.vers.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_TESTDIR([.], [.])

AC_PROG_INSTALL()
AC_CANONICAL_HOST()

AM_ICONV()

AC_MSG_CHECKING([cached host tuple])
AS_IF([{ test x"${ac_cv_host_system_type+set}" = x"set" &&
         test x"$ac_cv_host_system_type" != x"$host"; }],
      [AC_MSG_RESULT([different])
       AC_MSG_ERROR([remove config.cache and re-run configure])],
      [AC_MSG_RESULT(ok)])
ac_cv_host_system_type="$host"

AS_CASE([${host}],
  [*-*-darwin*|*-*-freebsd*|*-*-rhapsody*], [dnl
    PACKAGE_CHANGELOG_DATE="$(
      TZ=UTC date -v0H -v0M -v0S \
        -v${package_year}y \
        -v${package_month}m \
        -v${package_day}d \
        "${package_changelog_date_format}"
    )"],
  [*-*-openbsd*], [dnl
    PACKAGE_CHANGELOG_DATE="$(
      TZ=UTC date -j \
        "${package_changelog_date_format}" \
        ${package_year}${package_month}${package_day}0000
    )"],

    # Default: NetBSD, GNU core utilities (i.e. most Linux)
  [dnl
    PACKAGE_CHANGELOG_DATE="$(
      TZ=UTC date -d ${PACKAGE_DATE} \
        "${package_changelog_date_format}"
    )"])

AS_CASE([${host}], 

  ## Alpha (DEC) machines.
  [alpha*-dec-osf*], [dnl
    tcsh_config_file=decosf1
  ],



  [*-hp-hpux@<:@89@:>@*], [dnl
    tcsh_config_file=hpux8
  ],
  [*-hp-hpux1@<:@0-9@:>@*], [dnl
    tcsh_config_file=hpux11
  ],

  ## IBM AIX systems
  [*-ibm-aix*], [dnl
    tcsh_config_file=aix
  ],


  ## IBM OS/390 systems
  [*-ibm-os390*], [dnl
    CC='c89'
    tcsh_config_file=os390
  ],

  ## Android
  [*-*-android*], [dnl
    tcsh_config_file=android
  ],

  ## Linux
  [*-*-linux* | *-*-gnu* | *-*-k*bsd*-gnu], [dnl
    tcsh_config_file=linux
  ],


  ## Minix systems
  [*-*-minix*], [dnl
    tcsh_config_file=minix
  ],

  ## NetBSD systems
  [*-*-netbsd*], [dnl
    tcsh_config_file=bsd4.4
  ],

  ## FreeBSD systems
  [*-*-freebsd*], [dnl
    tcsh_config_file=bsd4.4
  ],

  ## MidnightBSD systems
  [*-*-midnightbsd*], [dnl
    tcsh_config_file=bsd4.4
  ],
  
  ## DragonFlyBSD systems
  [*-*-dragonfly*], [dnl
    tcsh_config_file=bsd4.4
  ],

  ## MirBSD systems
  [*-*-mirbsd*], [dnl
    tcsh_config_file=bsd4.4
  ],

  ## OpenBSD systems
  [*-*-openbsd*], [dnl
    tcsh_config_file=bsd4.4
  ],

  ## BSDI systems
  [*-*-bsdi*], [dnl
    tcsh_config_file=bsd4.4
  ],

  ## Mac OS X Server
  [*-*-rhapsody*], [dnl
    tcsh_config_file=bsd4.4
  ],

  ## Mac OS X Server
  [*-*-darwin*], [dnl
    tcsh_config_file=bsd4.4
  ],

  ## DragonFly systems
  [*-*-dragonfly*], [dnl
    tcsh_config_file=bsd4.4
  ],

  ## Silicon Graphics machines
  [*-sgi-iri*], [dnl
    tcsh_config_file=irix
    AS_CASE([${host_os}],
      [irix@<:@34@:>@*], [# Irix-3.x - Irix 4.x
        NON_GNU_DFLAGS='-D__STDC__'
        LIBS='-lsun -lbsd -lc_s'
      ],
      [irix5* | irix6.@<:@01@:>@*], [# Irix 5.x, Irix 6.0 - 6.1
        LIBS='-lbsd'
      ],
      [irix6.@<:@2-9@:>@*], [# Irix 6.2 and later
        tcsh_config_file=irix62
      ])
  ],

  [*-*-solaris2.@<:@01@:>@], [# Should handle sparc or x86
    tcsh_config_file=sol2
  ],
  [*-sun-solaris2.2], [# Sparc only release
    tcsh_config_file=sol22
  ],
  [*-sun-solaris2.3], [# Sparc only release
    tcsh_config_file=sol23
  ],
  [*-*-solaris2.@<:@45@:>@], [# Should handle sparc, x86 and powerpc
    tcsh_config_file=sol24
  ],
  [*-*-solaris2.@<:@678@:>@], [# Should handle sparc, x86 and powerpc
    tcsh_config_file=sol26
  ],
  [*-*-solaris2.*], [# Should handle sparc, x86 and powerpc
    tcsh_config_file=sol29
  ],



  ## Tektronix systems
  [m68k-tektronix-bsd*], [dnl
    tcsh_config_file=bsd
    NON_GNU_DFLAGS='-DBSD -DUTek'
  ],





  ## Red Hat Cygwin
  [*-cygwin], [dnl
    tcsh_config_file=cygwin
  ],



  ## QNX6
  [*-qnx6*], [dnl
    tcsh_config_file=qnx6
  ],

  [AC_MSG_ERROR([Tcsh can't guess the configuration file name
for `${host}' systems.
Check tcsh's `Ported' file for manual configuration instructions.])])

AC_MSG_NOTICE([using configuration file `$tcsh_config_file'])
cp ${srcdir}/system/${tcsh_config_file} config_p.h

AH_BOTTOM(
[#include "config_p.h"
#include "config_f.h"])

dnl Checks for programs
AC_PROG_CC()
AC_PROG_CPP()

AC_PATH_PROG([GENCAT], [gencat]) 

dnl Require build CC to create gethost helper when cross building
AS_IF([test "x${cross_compiling}" = xyes],
      [CC_FOR_GETHOST="cc"],
      [CC_FOR_GETHOST="\$(CC)"])
AC_SUBST(CC_FOR_GETHOST)

AS_IF([test "x${cross_compiling}" = xno],
      [CFLAGS_FOR_BUILD=${CFLAGS_FOR_BUILD:-"\$(CFLAGS)"}])
AC_SUBST(CFLAGS_FOR_BUILD)

AS_IF([test "x${cross_compiling}" = xno],
      [CPPFLAGS_FOR_BUILD=${CPPFLAGS_FOR_BUILD:-"\$(CPPFLAGS)"}])
AC_SUBST(CPPFLAGS_FOR_BUILD)

AS_IF([test "x${cross_compiling}" = xno],
      [LDFLAGS_FOR_BUILD=${LDFLAGS_FOR_BUILD:-"\$(LDFLAGS)"}])
AC_SUBST(LDFLAGS_FOR_BUILD)

AS_IF([test "x$GCC" != xyes],
      [DFLAGS="$DFLAGS $NON_GNU_DFLAGS"
       CFLAGS="$CFLAGS $NON_GNU_CFLAGS"])

dnl More recent Android requires PIEs
AS_CASE([${host}],
  [*-*-android*], [dnl
    CFLAGS="${CFLAGS} -fPIE"
    LDFLAGS="${LDFLAGS} -pie"
])

dnl Checks for libraries
dnl Modern glibc splits crypt into libxcrypt; search both library names.
AC_SEARCH_LIBS([crypt], [crypt xcrypt])
AC_SEARCH_LIBS(getspnam, sec)
AC_SEARCH_LIBS([tgetent], [termlib tinfo termcap curses ncurses], [], [
   AC_MSG_ERROR([unable to find the tgetent() function])
])
AC_SEARCH_LIBS(gethostbyname, nsl)
AC_SEARCH_LIBS(connect, socket)
AC_SEARCH_LIBS(catgets, catgets)
dnl Probe for POSIX glob(3) in libc; in-tree glob.c used as fallback.
AC_CHECK_FUNC([glob], [AC_DEFINE([HAVE_LIBC_GLOB], [1], [Define if the system provides a POSIX glob(3) function.])])

dnl Checks for header files
AC_CHECK_HEADERS([auth.h crypt.h features.h inttypes.h paths.h] dnl
                 [shadow.h stdint.h utmp.h utmpx.h])
AC_CHECK_HEADERS([wchar.h],
        [AC_CHECK_SIZEOF([wchar_t], [], [dnl
#include <stdio.h>
#include <wchar.h>
])
         AC_CHECK_HEADERS([wctype.h])])
AC_HEADER_DIRENT()
AC_HEADER_STAT()

dnl Checks for types
AC_CHECK_TYPES([long long])
AC_TYPE_GETGROUPS()
AC_TYPE_MODE_T()
AC_TYPE_SIZE_T()
AC_TYPE_UID_T()
AC_TYPE_UINT32_T()

AC_DEFUN([AC_TYPE_SSIZE_T], [
AC_CHECK_TYPE(ssize_t,,
AC_DEFINE(ssize_t, int, [Define to `int' not defined in <sys/types.h>.]), [dnl
#include <sys/types.h>
])
])
AC_TYPE_SSIZE_T()

AC_DEFUN([AC_TYPE_SOCKLEN_T], [
AC_CHECK_TYPE(socklen_t,,
AC_DEFINE(socklen_t, int, [Define to `int' if neither <sys/types.h> nor <sys/socket.h> define.]), [dnl
#include <sys/types.h>
#include <sys/socket.h>
])
])
AC_TYPE_SOCKLEN_T()

dnl Checks for structures
AC_CHECK_MEMBERS([struct dirent.d_ino], , , [dnl
#ifdef HAVE_DIRENT_H
# include <dirent.h>
#else
# ifdef HAVE_NDIR_H
#  include <ndir.h>
# else
#  include <sys/dir.h>
# endif
# define dirent direct
#endif
])
AC_CHECK_MEMBERS([struct utmp.ut_host, struct utmp.ut_user, struct utmp.ut_tv,
                  struct utmp.ut_xtime, struct utmpx.ut_host,
                  struct utmpx.ut_user, struct utmpx.ut_tv,
                  struct utmpx.ut_xtime], , , [dnl
#include <sys/types.h>
#ifdef HAVE_UTMPX_H
#include <utmpx.h>
#define utmp utmpx
#elif defined HAVE_UTMP_H
#include <utmp.h>
#endif
])
AC_CHECK_MEMBERS([struct sockaddr_storage.ss_family], , , [dnl
#include <sys/types.h>
#include <sys/socket.h>
])

dnl Checks for compiler characteristics
AC_C_CONST()
AC_C_VOLATILE()

dnl checks for library functions
AC_CHECK_DECLS([crypt, environ, gethostname, getpgrp], , , [dnl
#include "config_p.h"
AC_INCLUDES_DEFAULT([])
#ifdef HAVE_CRYPT_H
#include <crypt.h>
#endif
])
AC_CHECK_FUNC([setlocale], [have_setlocale=yes], [have_setlocale=no])
AC_CHECK_FUNC([catgets], [have_catgets=yes], [have_catgets=no])
AC_CHECK_FUNCS([dup2 getauthid getcwd gethostname getpwent] dnl
        [getutent getutxent mallinfo mallinfo2 mblen memmove memset] dnl
        [mkstemp nice setproctitle strtoll] dnl
        [nl_langinfo sbrk setpgid setpriority strerror strstr sysconf wcwidth])
AC_FUNC_GETPGRP()
AC_FUNC_MBRTOWC()
AS_IF([test "x${cross_compiling}" != xyes],
      [AC_FUNC_SETPGRP()],
      [ac_cv_func_setpgrp_void=yes])
AC_FUNC_STRCOLL()

AS_IF([test x"$ac_cv_func_sbrk" = x"yes"],
     [AC_MSG_CHECKING([for working sbrk])
      AC_RUN_IFELSE([AC_LANG_PROGRAM([[
#include <unistd.h>
]], [[
return sbrk(2048) == (void*)-1;
]])],
                    [AC_MSG_RESULT([yes])
                     AC_DEFINE([HAVE_WORKING_SBRK], [1], [Define to 1 if sbrk(N) works.])],
                    [AC_MSG_RESULT([no; use system malloc])],
                    [AC_MSG_RESULT([unknown - cross compiling])])],
     [AC_MSG_NOTICE([sbrk not present; use system malloc])])

dnl This is not good enough; we need sockaddr_storage too.
dnl See whether we can use IPv6 related functions
dnl AC_DEFUN([IPv6_CHECK_FUNC], [
dnl ac_tr_lib=HAVE_`echo $1 | sed -e 's/@<:@^a-zA-Z0-9_@:>@/_/g' \
dnl   -e 'y/abcdefghijklmnopqrstuvwxyz/ABCDEFGHIJKLMNOPQRSTUVWXYZ/'`
dnl AC_CHECK_FUNC($1, [dnl
dnl   AC_DEFINE_UNQUOTED($ac_tr_lib)
dnl   ac_cv_lib_inet6_$1=no], [dnl
dnl   AC_MSG_CHECKING([whether your system has IPv6 directory])
dnl   AC_CACHE_VAL(ipv6_cv_dir, [dnl
dnl     for ipv6_cv_dir in /usr/local/v6 /usr/inet6 no; do
dnl       if test $ipv6_cv_dir = no -o -d $ipv6_cv_dir; then
dnl     break
dnl       fi
dnl     done])dnl
dnl   AC_MSG_RESULT($ipv6_cv_dir)
dnl   if test $ipv6_cv_dir = no; then
dnl     ac_cv_lib_inet6_$1=no
dnl   else
dnl     if test x$ipv6_libinet6 = x; then
dnl       ipv6_libinet6=no
dnl       SAVELDFLAGS="$LDFLAGS"
dnl       LDFLAGS="-L$ipv6_cv_dir/lib"
dnl     fi
dnl     AC_CHECK_LIB(inet6, $1, [dnl
dnl       AC_DEFINE_UNQUOTED($ac_tr_lib)
dnl       if test $ipv6_libinet6 = no; then
dnl     ipv6_libinet6=yes
dnl     LIBS="$LIBS -linet6"
dnl       fi],)dnl
dnl     if test $ipv6_libinet6 = no; then
dnl       LDFLAGS="$SAVELDFLAGS"
dnl     fi
dnl   fi])dnl
dnl if test $ac_cv_func_$1 = yes -o $ac_cv_lib_inet6_$1 = yes
dnl then
dnl   ipv6_cv_$1=yes
dnl   ifelse([$2], , :, [$2])
dnl else
dnl   ipv6_cv_$1=no
dnl   ifelse([$3], , :, [$3])
dnl fi])
dnl IPv6_CHECK_FUNC(getnameinfo, DFLAGS="$DFLAGS -DINET6")

AC_SUBST(DFLAGS)

dnl Checks for system services
AS_IF([test "$have_setlocale" != no],
      [AC_ARG_ENABLE([nls], AS_HELP_STRING([--disable-nls], [Disable NLS support]),
                     [], [enable_nls=yes])
       AS_IF([test "x$enable_nls" != xno],
             [AC_DEFINE([NLS], [1], [Support NLS.])])])

AS_IF([{ test "x$enable_nls" != xno &&
         test "$have_catgets" != no &&
         test -n "$GENCAT"; }],
      [AC_ARG_ENABLE([nls-catalogs],
                     AS_HELP_STRING([--disable-nls-catalogs], [Disable NLS catalog support]),
                     [], [enable_nls_catalogs=yes])
       AS_IF([test "x$enable_nls_catalogs" != xno],
             [BUILD_CATALOGS="yes"
              AC_DEFINE([NLS_CATALOGS], [1], [Support NLS catalogs.])])])

AC_ARG_WITH(hesiod,
  [  --with-hesiod=PREFIX    Use Hesiod lookup for ~ expansion],
  [hesiod="$withval"], [hesiod=no])
AS_IF([test "$hesiod" != no],
      [HESLIB="-lhesiod"
       AC_CHECK_FUNC(res_send, :,
         AC_CHECK_LIB(resolv, res_send, HESLIB="$HESLIB -lresolv"))
       HESDEF=-DHESIOD
       AS_IF([test "$hesiod" != yes],
             [HESDEF="$HESDEF -I$hesiod/include"
              HESLIB="-L$hesiod/lib $HESLIB"])])
AC_SUBST(HESDEF)
AC_SUBST(HESLIB)
AC_SUBST(BUILD_CATALOGS)

dnl Note: the man-page template still lives on disk as `tcsh.man.in`
dnl (unchanged filename eases upstream diffs) but configure now emits
dnl the substituted output as `mcsh.man`, matching the installed brand.
AC_CONFIG_FILES([
    Makefile
    atlocal
    dch-template
    nls/Makefile
    patchlevel.h
    mcsh.man:tcsh.man.in
])
AC_OUTPUT()

AC_MSG_NOTICE([========= Configuration results =========])
AC_MSG_NOTICE([BUILD_CATALOGS          $BUILD_CATALOGS])
AC_MSG_NOTICE([CC                      $CC])
AC_MSG_NOTICE([CC_FOR_GETHOST          $CC_FOR_GETHOST])
AC_MSG_NOTICE([CFLAGS_FOR_BUILD        $CFLAGS_FOR_BUILD])
AC_MSG_NOTICE([CPPFLAGS_FOR_BUILD      $CPPFLAGS_FOR_BUILD])
AC_MSG_NOTICE([LDFLAGS_FOR_BUILD       $LDFLAGS_FOR_BUILD])
AC_MSG_NOTICE([CFLAGS                  $CFLAGS])
AC_MSG_NOTICE([CPP                     $CPP])
AC_MSG_NOTICE([CPPFLAGS                $CPPFLAGS])
AC_MSG_NOTICE([DFLAGS                  $DFLAGS])
AC_MSG_NOTICE([HESDEF                  $HESDEF])
AC_MSG_NOTICE([HESLIB                  $HESLIB])
AC_MSG_NOTICE([LDFLAGS                 $LDFLAGS])
AC_MSG_NOTICE([LIBICONV                $LIBICONV])
AC_MSG_NOTICE([LIBS                    $LIBS])
AC_MSG_NOTICE([PACKAGE_CHANGELOG_DATE  $PACKAGE_CHANGELOG_DATE])
AC_MSG_NOTICE([PACKAGE_DATE            $PACKAGE_DATE])
AC_MSG_NOTICE([PACKAGE_ENGLISH_DATE    $PACKAGE_ENGLISH_DATE])
AC_MSG_NOTICE([PACKAGE_PATCHLEVEL      $PACKAGE_PATCHLEVEL])
AC_MSG_NOTICE([PACKAGE_REV             $PACKAGE_REV])
AC_MSG_NOTICE([PACKAGE_VERS            $PACKAGE_VERS])
AC_MSG_NOTICE([PACKAGE_VERSION         $PACKAGE_VERSION])
AC_MSG_NOTICE([RELEASE_TAG             $RELEASE_TAG])
AC_MSG_NOTICE([tcsh_config_file        system/$tcsh_config_file])
AC_MSG_NOTICE([=========================================])
