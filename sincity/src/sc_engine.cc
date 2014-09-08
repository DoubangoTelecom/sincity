#include "sincity/sc_engine.h"

#include "tsk_debug.h"

#include "tinynet.h"

#include "tinydav.h"
#include "tinydav.h"

#include <assert.h>

#if SC_UNDER_WINDOWS
#	include "tinydav/tdav_win32.h"
#elif SC_UNDER_APPLE
#	include "tinydav/tdav_apple.h"
#endif

#if defined(_MSC_VER)
#	define snprintf		_snprintf
#	define vsnprintf	_vsnprintf
#	define strdup		_strdup
#	define stricmp		_stricmp
#	define strnicmp		_strnicmp
#else
#	if !HAVE_STRNICMP && !HAVE_STRICMP
#	define stricmp		strcasecmp
#	define strnicmp		strncasecmp
#	endif
#endif

bool SCEngine::s_bInitialized = false;

std::string SCEngine::s_strStunServerAddr = "stun.l.google.com";
unsigned short SCEngine::s_nStunServerPort = 19302;
std::string SCEngine::s_strStunUsername = "";
std::string SCEngine::s_strStunPassword = "";

SCEngine::SCEngine()
{

}

SCEngine::~SCEngine()
{

}

bool SCEngine::init()
{
    if (!s_bInitialized)
	{
        if (tnet_startup() != 0)
		{
            return false;
        }

        if (tdav_init() != 0)
		{
            return false;
        }
		
		SC_ASSERT((tmedia_defaults_set_profile(tmedia_profile_rtcweb)) == 0);
		SC_ASSERT((tmedia_defaults_set_avpf_mode(tmedia_mode_mandatory)) == 0);
		SC_ASSERT((tmedia_defaults_set_srtp_type(tmedia_srtp_type_dtls)) == 0);
		SC_ASSERT((tmedia_defaults_set_srtp_mode(tmedia_srtp_mode_mandatory)) == 0);
		SC_ASSERT((tmedia_defaults_set_ice_enabled(tsk_true)) == 0);

		SC_ASSERT((tmedia_defaults_set_pref_video_size(tmedia_pref_video_size_vga)) == 0);
		SC_ASSERT((tmedia_defaults_set_video_fps(10)) == 0);

		SC_ASSERT((tdav_set_codecs((tdav_codec_id_t)(tmedia_codec_id_vp8))) == 0);
		SC_ASSERT((tdav_codec_set_priority((tdav_codec_id_t)tmedia_codec_id_vp8, 0)) == 0);

        s_bInitialized = true;
    }

    return true;
}

bool SCEngine::deInit()
{
    if (s_bInitialized)
	{
        tdav_deinit();
        tnet_cleanup();

        s_bInitialized = false;
    }

    return true;
}

bool SCEngine::setDebugLevel(SCDebugLevel_t eLevel)
{
    tsk_debug_set_level((int)eLevel);
    return true;
}

bool SCEngine::setSSLCertificates(const char* strPublicKey, const char* strPrivateKey /*= NULL*/, const char* strCA /*= NULL*/, bool bMutualAuth /*= false*/)
{
#if SC_UNDER_WINDOWS
	char ssl_file_priv[MAX_PATH] = { '\0' };
	char ssl_file_pub[MAX_PATH] = { '\0' };
	char ssl_file_ca[MAX_PATH] = { '\0' };
#define _file_exists(path) tsk_plugin_file_exist((path))

	if (strPublicKey)
	{
		if (!_file_exists(strPublicKey))
		{
			snprintf(ssl_file_pub, sizeof(ssl_file_pub), "%s/%s", tdav_get_current_directory_const(), strPublicKey);
			strPublicKey = ssl_file_pub;
		}
	}
	if (strPrivateKey)
	{
		if (!_file_exists(strPrivateKey))
		{
			snprintf(ssl_file_priv, sizeof(ssl_file_priv), "%s/%s", tdav_get_current_directory_const(), strPrivateKey);
			strPrivateKey = ssl_file_priv;
		}
	}
	if (strCA)
	{
		if (!_file_exists(strCA))
		{
			snprintf(ssl_file_ca, sizeof(ssl_file_ca), "%s/%s", tdav_get_current_directory_const(), strCA);
			strCA = ssl_file_ca;
		}
	}
#endif /* SC_UNDER_WINDOWS */

	return (tmedia_defaults_set_ssl_certs(strPrivateKey, strPublicKey, strCA, bMutualAuth ? tsk_true : tsk_false) == 0);
}
