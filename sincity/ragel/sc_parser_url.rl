#include "sincity/sc_parser_url.h"
#include "sincity/sc_url.h"
#include "sincity/sc_debug.h"

#include "tsk_ragel_state.h"
#include "tsk_string.h"
#include "tsk_memory.h"

%%{
	machine sc_machine_parser_url;

	# Includes
	include sc_machine_utils "./ragel/sc_machine_utils.rl";
			
	action tag{
		tag_start = p;
	}

	#/* Sets URL type */
	action is_tcp { scheme = tsk_strdup("tcp"), eType = SCUrlType_TCP; }
	action is_tls { scheme = tsk_strdup("tls"), eType = SCUrlType_TLS; }
	action is_http { scheme = tsk_strdup("http"), eType = SCUrlType_HTTP; }
	action is_https { scheme = tsk_strdup("https"), eType = SCUrlType_HTTPS; }
	action is_ws { scheme = tsk_strdup("ws"), eType = SCUrlType_WS; }
	action is_wss { scheme = tsk_strdup("wss"), eType = SCUrlType_WSS; }

	#/* Sets HOST type */
	action is_ipv4 { eHostType = SCUrlHostType_IPv4; }
	action is_ipv6 { eHostType = SCUrlHostType_IPv6; }
	action is_hostname { eHostType = SCUrlHostType_Hostname; }

	action parse_host{
		TSK_PARSER_SET_STRING(host);
	}

	action parse_port{
		have_port = tsk_true;
		TSK_PARSER_SET_INT(port);
	}

	action parse_hpath{
		TSK_PARSER_SET_STRING(hpath);
	}

	action parse_search{
		TSK_PARSER_SET_STRING(search);
	}

	action eob{
	}

	#// RFC 1738: "http://" hostport [ "/" hpath [ "?" search ]]
	#// FIXME: hpath is no optional (see above) but in my def. I use it as opt (any*).

	search = any* >tag %parse_search;
	hpath = any* >tag %parse_hpath;
	port = DIGIT+ >tag %parse_port;
	myhost = ((IPv6reference >is_ipv6) | (IPv4address >is_ipv4) | (hostname >is_hostname)) >tag %parse_host;
	hostport = myhost ( ":" port )?;
	main := ( (("tcp:"i>tag %is_tcp | "tls:"i>tag %is_tls | "ws:"i>tag %is_ws | "wss:"i>tag %is_wss | "http:"i>tag %is_http | "https:"i>tag %is_https) "//")? hostport? :>("/" hpath :>("?" search)?)? ) @eob;
	#main := ( hostport? :>("/" hpath :>("?" search)?)? ) @eob;
	
}%%

SCObjWrapper<SCUrl*> sc_url_parse(const char *urlstring, size_t length)
{
	SCObjWrapper<SCUrl*> oUrl;
	tsk_bool_t have_port = tsk_false;
	int cs = 0;
	const char *p = urlstring;
	const char *pe = p + length;
	const char *eof = pe;

	const char *ts = 0, *te = 0;
	int act = 0;
	
	const char *tag_start = 0;

	SCUrlType_t eType = SCUrlType_None;
	SCUrlHostType_t eHostType = SCUrlHostType_None;
	char* scheme = tsk_null;
	char* host = tsk_null;
	char* hpath = tsk_null;
	char* search = tsk_null;
	unsigned short port = 0;
	
	TSK_RAGEL_DISABLE_WARNINGS_BEGIN()
	%%write data;
	(void)(ts);
	(void)(te);
	(void)(act);
	(void)(eof);
	(void)(sc_machine_parser_url_first_final);
	(void)(sc_machine_parser_url_error);
	(void)(sc_machine_parser_url_en_main);
	%%write init;
	%%write exec;
	TSK_RAGEL_DISABLE_WARNINGS_END()
	
	if ( cs < %%{ write first_final; }%% ){
		SC_DEBUG_ERROR("Failed to parse URL: '%.*s'", (int)length, urlstring);
		goto bail;
	}
	else if (!have_port) {
		if (eType == SCUrlType_HTTPS || eType == SCUrlType_WSS || eType == SCUrlType_TLS) {
			port = 443;
		}
		else {
			port = 80;
		}
	}

	oUrl = SCUrl::newObj(eType, scheme, host, hpath, search, port, eHostType);
	
bail:
	TSK_FREE(scheme);
	TSK_FREE(host);
	TSK_FREE(hpath);
	TSK_FREE(search);

	return oUrl;
}
