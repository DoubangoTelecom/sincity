sbin_PROGRAMS = sintest

sintest_CPPFLAGS = \
${TINYSAK_CFLAGS} \
${TINYNET_CFLAGS} \
${TINYHTTP_CFLAGS} \
${TINYDAV_CFLAGS} \
${TINYSDP_CFLAGS} \
${TINYBFCP_CFLAGS} \
${TINYSIGCOMP_CFLAGS} \
${TINYMEDIA_CFLAGS} \
${TINYMSRP_CFLAGS} \
${TINYRTP_CFLAGS} \
${TINYIPSEC_CFLAGS} \
${LIBSINCITY_CFLAGS} \
${DOUBANGO_LIBS_FALLBACK} \
-Iinclude

sintest_LDADD = \
${TINYSAK_LIBS} \
${TINYNET_LIBS} \
${TINYHTTP_LIBS} \
${TINYDAV_LIBS} \
${TINYSDP_LIBS} \
${TINYBFCP_LIBS} \
${TINYMEDIA_LIBS} \
${TINYMSRP_LIBS} \
${TINYRTP_LIBS} \
${TINYIPSEC_LIBS} \
${LIBSINCITY_LIBS} \
${DOUBANGO_LIBS_FALLBACK}

sintest_SOURCES = main.cc

samples:
	cp -f ./SSL_CA.pem ${exec_prefix}/sbin
	cp -f ./SSL_Priv.pem ${exec_prefix}/sbin
	cp -f ./SSL_Pub.pem ${exec_prefix}/sbin
	cp -f ./config.json ${exec_prefix}/sbin
