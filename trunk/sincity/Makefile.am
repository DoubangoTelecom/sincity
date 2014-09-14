lib_LTLIBRARIES         = libsincity.la
libsincity_la_LIBADD = \
	${DOUBANGO_LIBS_FALLBACK} \
	${TINYSAK_LIBS} \
	${TINYNET_LIBS} \
	${TINYHTTP_LIBS} \
	${TINYDAV_LIBS} \
	${TINYSDP_LIBS} \
	${TINYBFCP_LIBS} \
	${TINYMEDIA_LIBS} \
	${TINYMSRP_LIBS} \
	${TINYRTP_LIBS} \
	${TINYIPSEC_LIBS}

libsincity_la_CPPFLAGS = \
	-Iinclude \
	${TINYSAK_CFLAGS} \
	${TINYNET_CFLAGS} \
	${TINYHTTP_CFLAGS} \
	${TINYDAV_CFLAGS} \
	${TINYSDP_CFLAGS} \
	${TINYBFCP_CFLAGS} \
	${TINYMEDIA_CFLAGS} \
	${TINYMSRP_CFLAGS} \
	${TINYRTP_CFLAGS} \
	${TINYIPSEC_CFLAGS}

libsincity_la_LDFLAGS = ${LDFLAGS}

libsincity_la_SOURCES  = \
	src/sc_engine.cc \
	src/sc_jsoncpp.cc \
	src/sc_mutex.cc \
	src/sc_nettransport.cc \
	src/sc_nettransport_ws.cc \
	src/sc_obj.cc \
	src/sc_parser_url.cc \
	src/sc_session.cc \
	src/sc_session_call.cc \
	src/sc_signaling.cc \
	src/sc_url.cc \
	src/sc_utils.cc

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libsincity.pc
