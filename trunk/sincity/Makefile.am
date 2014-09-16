lib_LTLIBRARIES         = libsincity.la
libsincity_la_LIBADD = \
	${DOUBANGO_LIBS_FALLBACK} \
	${TINYSAK_LIBS} \
	${TINYNET_LIBS} \
	${TINYHTTP_LIBS} \
	${TINYDAV_LIBS} \
	${TINYSDP_LIBS} \
	${TINYBFCP_LIBS} \
	${sincity_LIBS} \
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
	${sincity_CFLAGS} \
	${TINYMSRP_CFLAGS} \
	${TINYRTP_CFLAGS} \
	${TINYIPSEC_CFLAGS}

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
	
libsincity_la_LDFLAGS = $LDFLAGS -no-undefined

_includedir = $(includedir)/sincity
_include_HEADERS = include/*.h
__includedir = $(includedir)/sincity/sincity
__include_HEADERS = include/sincity/*.h
jsoncpp_includedir = $(includedir)/sincity/sincity/jsoncpp
jsoncpp_include_HEADERS = include/sincity/jsoncpp/*.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libsincity.pc

if CROSS_COMPILING
install-data-hook:
	echo "Cross-compilation is active. Do not run ldconfig";
else
install-data-hook:
	$(MKDIR_P) $(DESTDIR)/etc/ld.so.conf.d
	echo "$(libdir)" > $(DESTDIR)/etc/ld.so.conf.d/libsincity.conf
	ldconfig
endif

if CROSS_COMPILING
	echo "Cross-compilation is active. Do not remove libs";
else
uninstall-hook:
	rm -f $(DESTDIR)/etc/ld.so.conf.d/libsincity.conf
endif