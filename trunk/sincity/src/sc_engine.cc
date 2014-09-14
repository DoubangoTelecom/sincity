#include "sincity/sc_engine.h"
#include "sincity/sc_utils.h"

#include "tsk_debug.h"

#include "tinynet.h"

#include "tinydav/tdav.h"
#include "tinymedia.h"

#include <assert.h>

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

std::string SCEngine::s_strCredUserId = "";
std::string SCEngine::s_strCredPassword = "";


SCEngine::SCEngine()
{

}

SCEngine::~SCEngine()
{

}

bool SCEngine::init(std::string strCredUserId, std::string strCredPassword /*= ""*/)
{
    if (strCredUserId.empty()) {
        SC_DEBUG_ERROR("Invalid argument");
        return false;
    }

    if (!s_bInitialized) {
        if (tnet_startup() != 0) {
            return false;
        }

        if (tdav_init() != 0) {
            return false;
        }

        SC_ASSERT(tmedia_defaults_set_profile(tmedia_profile_rtcweb) == 0);
        SC_ASSERT(tmedia_defaults_set_avpf_mode(tmedia_mode_mandatory) == 0);
        SC_ASSERT(tmedia_defaults_set_srtp_type(tmedia_srtp_type_dtls) == 0);
        SC_ASSERT(tmedia_defaults_set_srtp_mode(tmedia_srtp_mode_mandatory) == 0);
        SC_ASSERT(tmedia_defaults_set_ice_enabled(tsk_true) == 0);

		SC_ASSERT(tmedia_defaults_set_pref_video_size(tmedia_pref_video_size_vga) == 0);
        SC_ASSERT(tmedia_defaults_set_video_fps(10) == 0);

        SC_ASSERT(tdav_set_codecs((tdav_codec_id_t)(tmedia_codec_id_vp8)) == 0);
        SC_ASSERT(tdav_codec_set_priority((tdav_codec_id_t)tmedia_codec_id_vp8, 0) == 0);

        s_bInitialized = true;
    }

	s_strCredUserId = strCredUserId;
    s_strCredPassword = strCredPassword;

    return true;
}

bool SCEngine::deInit()
{
    if (s_bInitialized) {
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

    if (strPublicKey) {
		if (!SCUtils::fileExists(strPublicKey)) {
            snprintf(ssl_file_pub, sizeof(ssl_file_pub), "%s/%s", SCUtils::currentDirectoryPath(), strPublicKey);
            strPublicKey = ssl_file_pub;
        }
    }
    if (strPrivateKey) {
        if (!SCUtils::fileExists(strPrivateKey)) {
            snprintf(ssl_file_priv, sizeof(ssl_file_priv), "%s/%s", SCUtils::currentDirectoryPath(), strPrivateKey);
            strPrivateKey = ssl_file_priv;
        }
    }
    if (strCA) {
        if (!SCUtils::fileExists(strCA)) {
            snprintf(ssl_file_ca, sizeof(ssl_file_ca), "%s/%s", SCUtils::currentDirectoryPath(), strCA);
            strCA = ssl_file_ca;
        }
    }
#endif /* SC_UNDER_WINDOWS */

    return (tmedia_defaults_set_ssl_certs(strPrivateKey, strPublicKey, strCA, bMutualAuth ? tsk_true : tsk_false) == 0);
}

bool SCEngine::setVideoPrefSize(const char* strPrefVideoSize)
{
	SC_ASSERT(strPrefVideoSize);
	int i;
    struct pref_video_size { const char* name; tmedia_pref_video_size_t size; size_t w; size_t h;};
    static const pref_video_size pref_video_sizes[] =
    {
            {"sqcif", tmedia_pref_video_size_sqcif, 128, 98}, // 128 x 98
            {"qcif", tmedia_pref_video_size_qcif, 176, 144}, // 176 x 144
            {"qvga", tmedia_pref_video_size_qvga, 320, 240}, // 320 x 240
            {"cif", tmedia_pref_video_size_cif, 352, 288}, // 352 x 288
            {"hvga", tmedia_pref_video_size_hvga, 480, 320}, // 480 x 320
            {"vga", tmedia_pref_video_size_vga, 640, 480}, // 640 x 480
            {"4cif", tmedia_pref_video_size_4cif, 704, 576}, // 704 x 576
            {"svga", tmedia_pref_video_size_svga, 800, 600}, // 800 x 600
            {"480p", tmedia_pref_video_size_480p, 852, 480}, // 852 x 480
            {"720p", tmedia_pref_video_size_720p, 1280, 720}, // 1280 x 720
            {"16cif", tmedia_pref_video_size_16cif, 1408, 1152}, // 1408 x 1152
            {"1080p", tmedia_pref_video_size_1080p, 1920, 1080}, // 1920 x 1080
            {"2160p", tmedia_pref_video_size_2160p, 3840, 2160}, // 3840 x 2160
    };
    static const int pref_video_sizes_count = sizeof(pref_video_sizes)/sizeof(pref_video_sizes[0]);
    
    for (i = 0; i < pref_video_sizes_count; ++i)
    {
            if (tsk_striequals(pref_video_sizes[i].name, strPrefVideoSize))
            {
				if (tmedia_defaults_set_pref_video_size(pref_video_sizes[i].size) == 0)
                {
                    return true;
                }
            }
    }
    SC_DEBUG_ERROR("%s not valid as video size. Valid values: ...", strPrefVideoSize);
    return false;
}

bool SCEngine::setVideoFps(int fps)
{
	return ((tmedia_defaults_set_video_fps(fps) == 0));
}

bool SCEngine::setVideoBandwidthUpMax(int bandwwidthMax)
{
	return ((tmedia_defaults_set_bandwidth_video_upload_max(bandwwidthMax) == 0));
}

bool SCEngine::setVideoBandwidthDownMax(int bandwwidthMax)
{
	return ((tmedia_defaults_set_bandwidth_video_download_max(bandwwidthMax) == 0));
}

bool SCEngine::setVideoMotionRank(int motionRank)
{
	return ((tmedia_defaults_set_video_motion_rank(motionRank) == 0));
}

bool SCEngine::setVideoCongestionCtrlEnabled(bool congestionCtrl)
{
	return ((tmedia_defaults_set_congestion_ctrl_enabled(congestionCtrl ? tsk_true : tsk_false) == 0));
}

bool SCEngine::setNattStunServer(const char* host, unsigned short port /*= 3478*/)
{
	return ((tmedia_defaults_set_stun_server(host, port) == 0));
}

bool SCEngine::setNattStunCredentials(const char* username, const char* password)
{
	return ((tmedia_defaults_set_stun_cred(username, password) == 0));
}

bool SCEngine::setNattIceStunEnabled(bool enabled)
{
	return ((tmedia_defaults_set_icestun_enabled(enabled ? tsk_true : tsk_false) == 0));
}

bool SCEngine::setNattIceTurnEnabled(bool enabled)
{
	return ((tmedia_defaults_set_iceturn_enabled(enabled ? tsk_true : tsk_false) == 0));
}
