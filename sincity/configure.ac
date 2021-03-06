#
# Copyright (C) 2013-2015 Mamadou DIOP
# Copyright (C) 2013-2015 Doubango Telecom <http://www.doubango.org>
# License: GPLv3 or commercial
# This file is part of Sincity <https://code.google.com/p/sincity/>
#

AC_PREREQ([2.0])
AC_INIT(libsincity, 1.2.0, opensincity(at)googlegroups(dot)com)
AM_INIT_AUTOMAKE
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([src/sc_engine.cc])

dnl find suitable C and C++ compilers
AC_PROG_CC
AC_PROG_CXX
AM_PROG_CC_C_O

AC_PROG_INSTALL

AC_PROG_LIBTOOL
AC_LIBTOOL_WIN32_DLL

AC_PROG_SED

AC_PROG_MAKE_SET

PKG_PROG_PKG_CONFIG

AC_SUBST(DOUBANGO_MINVER, 2.0.1316)

target_cpu=`uname -m`
target_os=`uname -s`
#echo Setting up build environment for ${target_cpu}${target_os}

AC_CHECK_HEADERS([unistd.h spawn.h sys/wait.h errno.h])
AC_CHECK_LIB(c, dlopen, AC_SUBST(LIBDL,""), AC_CHECK_LIB(dl, dlopen, AC_SUBST(LIBDL,-ldl)))
AC_CHECK_FUNCS([fork vfork execl posix_spawn waitpid setpgid popen pclose])

### ENABLE-DEBUG
debug_default="no"
AC_ARG_ENABLE(debug, 
[  --enable-debug[=no/yes] turn on/off debugging 
                       [[default=$debug_default]]],
[ if test "x$enableval" = "xyes" ; then
	CFLAGS="$CFLAGS -O0 -g3 -DDEBUG"
	AC_MSG_RESULT(yes)
  else
	CFLAGS="$CFLAGS -O3 -NDEBUG"
	AC_MSG_RESULT(no)
  fi
],
[])


###########
# DOUBANGO
###########
AC_SUBST(have_tinysak, no)
AC_SUBST(have_tinynet, no)
AC_SUBST(have_tinyhttp, no)
AC_SUBST(have_tinydav, no)
AC_SUBST(have_tinysdp, no)
AC_SUBST(have_tinybfcp, no)
AC_SUBST(have_tinymedia, no)
AC_SUBST(have_tinyipsec, no)
AC_SUBST(have_tinymsrp, no)
AC_SUBST(have_tinyrtp, no)

PKG_CHECK_MODULES(TINYSAK, tinySAK >= ${DOUBANGO_MINVER}, AC_SUBST(have_tinysak, yes), AC_MSG_WARN(Cannot find tinySAK >= ${DOUBANGO_MINVER} using pkg-config))
PKG_CHECK_MODULES(TINYNET, tinyNET >= ${DOUBANGO_MINVER}, AC_SUBST(have_tinynet, yes), AC_MSG_WARN(Cannot find tinyNET >= ${DOUBANGO_MINVER} using pkg-config))
PKG_CHECK_MODULES(TINYHTTP, tinyHTTP >= ${DOUBANGO_MINVER}, AC_SUBST(have_tinyhttp, yes), AC_MSG_WARN(Cannot find tinyHTTP >= ${DOUBANGO_MINVER} using pkg-config))
PKG_CHECK_MODULES(TINYDAV, tinyDAV >= ${DOUBANGO_MINVER}, AC_SUBST(have_tinydav, yes), AC_MSG_WARN(Cannot find tinyDAV >= ${DOUBANGO_MINVER} using pkg-config))
PKG_CHECK_MODULES(TINYSDP, tinySDP >= ${DOUBANGO_MINVER}, AC_SUBST(have_tinysdp, yes), AC_MSG_WARN(Cannot find tinySDP >= ${DOUBANGO_MINVER} using pkg-config))
PKG_CHECK_MODULES(TINYBFCP, tinyBFCP >= ${DOUBANGO_MINVER}, AC_SUBST(have_tinybfcp, yes), AC_MSG_WARN(Cannot find tinyBFCP >= ${DOUBANGO_MINVER} using pkg-config))
PKG_CHECK_MODULES(TINYMEDIA, tinyMEDIA >= ${DOUBANGO_MINVER}, AC_SUBST(have_tinymedia, yes), AC_MSG_WARN(Cannot find tinyMEDIA >= ${DOUBANGO_MINVER} using pkg-config))
PKG_CHECK_MODULES(TINYMSRP, tinyMSRP >= ${DOUBANGO_MINVER}, AC_SUBST(have_tinymsrp, yes), AC_MSG_WARN(Cannot find tinyMSRP >= ${DOUBANGO_MINVER} using pkg-config))
PKG_CHECK_MODULES(TINYRTP, tinyRTP >= ${DOUBANGO_MINVER}, AC_SUBST(have_tinyrtp, yes), AC_MSG_WARN(Cannot find tinyRTP >= ${DOUBANGO_MINVER} using pkg-config))
PKG_CHECK_MODULES(TINYIPSEC, tinyIPSec >= ${DOUBANGO_MINVER}, AC_SUBST(have_tinyipsec, yes), AC_MSG_WARN(Cannot find tinyIPSec >= ${DOUBANGO_MINVER} using pkg-config))

if test $have_tinysak:$have_tinynet:$have_tinyhttp:$have_tinydav:$have_tinysdp:$have_tinybfcp:$have_tinymedia:$have_tinymsrp:$have_tinyrtp:$have_tinyipsec != yes:yes:yes:yes:yes:yes:yes:yes:yes:yes ; then
	AC_MSG_WARN([
***
*** Failed to find all DOUBANGO libraries using pkg-config.
*** This most likely means the DOUBANGO pkg files (*.pc) are not 
*** in the default path where pkg-config expect them.
***
*** Please set your PKG_CONFIG_PATH environment variable to include the
*** directory in which the *.pc files are installed. Most likely
*** these files are installed in /usr/local/lib/pkgconfig; if that is the
*** case, try:
***
***     export PKG_CONFIG_PATH="\$PKG_CONFIG_PATH:/usr/local/lib/pkgconfig"
***
*** then run this ./configure <your options here> again.
***
*** or, try:
***
***   PKG_CONFIG_PATH=/usr/local/lib/pkgconfig ./configure <your options here>
***
*** Anyways, we're trying another way to find DOUBANGO.
***
]);

AC_SUBST(DOUBANGO_INCLUDES_PATH,/usr/local/include)
AC_SUBST(DOUBANGO_LIBS_FALLBACK,-L/usr/local/lib)
AC_SUBST(TINYSAK_LIBS,-ltinySAK) AC_SUBST(TINYSAK_CFLAGS,-I${DOUBANGO_INCLUDES_PATH}/tinysak)
AC_SUBST(TINYNET_LIBS,-ltinyNET) AC_SUBST(TINYNET_CFLAGS,-I${DOUBANGO_INCLUDES_PATH}/tinynet)
AC_SUBST(TINYHTTP_LIBS,-ltinyHTTP) AC_SUBST(TINYHTTP_CFLAGS,-I${DOUBANGO_INCLUDES_PATH}/tinyhttp)
AC_SUBST(TINYDAV_LIBS,-ltinyDAV) AC_SUBST(TINYDAV_CFLAGS,-I${DOUBANGO_INCLUDES_PATH}/tinydav)
AC_SUBST(TINYSDP_LIBS,-ltinySDP) AC_SUBST(TINYSDP_CFLAGS,-I${DOUBANGO_INCLUDES_PATH}/tinysdp)
AC_SUBST(TINYBFCP_LIBS,-ltinyBFCP) AC_SUBST(TINYBFCP_CFLAGS,-I${DOUBANGO_INCLUDES_PATH}/tinybfcp)
AC_SUBST(TINYMEDIA_LIBS,-ltinyMEDIA) AC_SUBST(TINYMEDIA_CFLAGS,-I${DOUBANGO_INCLUDES_PATH}/tinymedia)
AC_SUBST(TINYMSRP_LIBS,-ltinyMSRP) AC_SUBST(TINYMSRP_CFLAGS,-I${DOUBANGO_INCLUDES_PATH}/tinymsrp)
AC_SUBST(TINYRTP_LIBS,-ltinyRTP) AC_SUBST(TINYRTP_CFLAGS,-I${DOUBANGO_INCLUDES_PATH}/tinyrtp)
AC_SUBST(TINYIPSEC_LIBS,-ltinyIPSec) AC_SUBST(TINYIPSEC_CFLAGS,-I${DOUBANGO_INCLUDES_PATH}/tinyipsec)
fi

if test $have_tinysak != yes; then
	AC_CHECK_HEADERS(tinysak/tinysak_config.h, 
		AC_CHECK_LIB(tinySAK, tsk_object_new, AC_SUBST(have_tinysak, yes), AC_MSG_ERROR(Failed to find libtinySAK), [${LIBDL}])
	)
fi
if test $have_tinynet != yes; then
	AC_CHECK_HEADERS(tinynet/tinynet_config.h, 
		AC_CHECK_LIB(tinyNET, tnet_startup, AC_SUBST(have_tinynet, yes), AC_MSG_ERROR(Failed to find libtinyNET), [-ltinySAK ${LIBDL}])
	)
fi
if test $have_tinyhttp != yes; then
	AC_CHECK_HEADERS(tinyhttp/tinyhttp_config.h, 
		AC_CHECK_LIB(tinyHTTP, thttp_stack_create, AC_SUBST(have_tinyhttp, yes), AC_MSG_ERROR(Failed to find libtinyHTTP), [-ltinySAK -ltinyNET ${LIBDL}])
	)
fi
if test $have_tinydav != yes; then
	AC_CHECK_HEADERS(tinydav/tinydav_config.h, 
		AC_CHECK_LIB(tinyDAV, tdav_init, AC_SUBST(have_tinydav, yes), AC_MSG_ERROR(Failed to find libtinyDAV), [-ltinySAK -ltinyNET -ltinySDP -ltinyRTP -ltinyMEDIA -ltinyMSRP ${LIBDL}])
	)
fi
if test $have_tinysdp != yes; then
	AC_CHECK_HEADERS(tinysdp/tinysdp_config.h, 
		AC_CHECK_LIB(tinySDP, tsdp_message_create, AC_SUBST(have_tinysdp, yes), AC_MSG_ERROR(Failed to find libtinySDP), [-ltinySAK ${LIBDL}])
	)
fi
if test $have_tinybfcp != yes; then
	AC_CHECK_HEADERS(tinybfcp/tinybfcp_config.h, 
		AC_CHECK_LIB(tinyBFCP, tbfcp_pkt_create, AC_SUBST(have_tinybfcp, yes), AC_MSG_ERROR(Failed to find libtinyBFCP), [-ltinySAK -ltinyNET ${LIBDL}])
	)
fi
if test $have_tinymedia != yes; then
	AC_CHECK_HEADERS(tinymedia/tinymedia_config.h, 
		AC_CHECK_LIB(tinyMEDIA, tmedia_session_mgr_create, AC_SUBST(have_tinymedia, yes), AC_MSG_ERROR(Failed to find libtinyMEDIA), [-ltinySAK -ltinyNET -ltinySDP ${LIBDL}])
	)
fi
if test $have_tinymsrp != yes; then
	AC_CHECK_HEADERS(tinymsrp/tinymsrp_config.h, 
		AC_CHECK_LIB(tinyMSRP, tmsrp_create_report, AC_SUBST(have_tinymsrp, yes), AC_MSG_ERROR(Failed to find libtinyMSRP), [-ltinySAK -ltinyNET -ltinySDP -ltinyHTTP ${LIBDL}])
	)
fi
if test $have_tinyrtp != yes; then
	AC_CHECK_HEADERS(tinyrtp/tinyrtp_config.h, 
		AC_CHECK_LIB(tinyRTP, trtp_manager_create, AC_SUBST(have_tinyrtp, yes), AC_MSG_ERROR(Failed to find libtinyRTP), [-ltinySAK -ltinyNET -ltinyMEDIA ${LIBDL}])
	)
fi
if test $have_tinyipsec != yes; then
	AC_CHECK_HEADERS(tinyipsec/tinyipsec_config.h, 
		AC_CHECK_LIB(tinyIPSec, tipsec_ctx_create, AC_SUBST(have_tinyipsec, yes) , AC_MSG_ERROR(Failed to find libtinyIPSec), [-ltinySAK ${LIBDL}])
	)
fi

AC_OUTPUT(
Makefile
libsincity.pc
)


AC_MSG_NOTICE([
 
**************************************************************************
 				CONGRATULATIONS
**************************************************************************
Target OS:                                     ${target_os}
Target CPU:                                    ${target_cpu}
Host setup:                                    ${host}
Install prefix:                                ${prefix}
Compiler:                                      ${CC}

DOUBANGO
     MIN_VER:                                  ${DOUBANGO_MINVER}
     TINYSAK:                                  ${have_tinysak}
     TINYNET:                                  ${have_tinynet}
     TINYHTTP:                                 ${have_tinyhttp}
     TINYDAV:                                  ${have_tinydav}
     TINYSDP:                                  ${have_tinysdp}
     TINYBFCP:                                 ${have_tinybfcp}
     TINYMEDIA:                                ${have_tinymedia}
     TINYMSRP:                                 ${have_tinymsrp}
     TINYRTP:                                  ${have_tinyrtp}
     TINYIPSEC:                                ${have_tinyipsec}

Report issues at https://groups.google.com/group/sincity

-------------------------------------------------------------------------
Next steps
  1) run 'make' to build the source
  2) run 'make install' to install binaries
-------------------------------------------------------------------------

])






