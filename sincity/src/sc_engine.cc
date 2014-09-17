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

/**@defgroup _Group_CPP_Engine Engine
* @brief Static class used to configure the media and signaling engines.
*/


/*
* Constructor for the engine.
* Must never be called.
*/
SCEngine::SCEngine()
{

}

/*
* Destructor for the engine.
*/
SCEngine::~SCEngine()
{

}

/**@ingroup _Group_CPP_Engine
* Initializes the media and network layers. This function must be the first one to call.
* @param strCredUserId User (or device) identifer. This parameter is required and must not be null or empty.
* @param strCredPassword User (or device) password. This parameter is optional.
* @retval <b>true</b> if no error; otherwise <b>false</b>.
*/
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
#if SC_UNDER_WINDOWS
#else
		SC_ASSERT(tmedia_producer_set_friendly_name(tmedia_video, "/dev/video") == 0);
		SC_ASSERT(tmedia_producer_set_friendly_name(tmedia_bfcp_video, "/dev/video") == 0);
#endif

        SC_ASSERT(tdav_set_codecs((tdav_codec_id_t)(tmedia_codec_id_vp8)) == 0);
        SC_ASSERT(tdav_codec_set_priority((tdav_codec_id_t)tmedia_codec_id_vp8, 0) == 0);

        s_bInitialized = true;
    }

    s_strCredUserId = strCredUserId;
    s_strCredPassword = strCredPassword;

    return true;
}

/**@ingroup _Group_CPP_Engine
* Cleanups the media and network layers. This function must be the last one to call.
* @retval <b>true</b> if no error; otherwise <b>false</b>.
*/
bool SCEngine::deInit()
{
    if (s_bInitialized) {
        tdav_deinit();
        tnet_cleanup();

        s_bInitialized = false;
    }

    return true;
}

/**@ingroup _Group_CPP_Engine
* Sets the debug level.
* @param eLevel The new debug level.
* @retval <b>true</b> if no error; otherwise <b>false</b>.
*/
bool SCEngine::setDebugLevel(SCDebugLevel_t eLevel)
{
    tsk_debug_set_level((int)eLevel);
    return true;
}

/**@ingroup _Group_CPP_Engine
* Sets the paths to the SSL certificates used to secure the signaling (WebSocket) an media (DTLS-SRTP) layers.
* The library comes with default self-signed certificates: <b>SSL_Pub.pem</b>, <b>SSL_Priv.pem</b> and <b>SSL_CA.pem</b>. These certificates could be used for testing.
* @param strPublicKey Path (relative or absolute) to the public SSL key (PEM format) used for both DTLS-SRTP (media) and WSS (signaling) connections. <b>Required</b>.
* @param strPrivateKey Path (relative or absolute) to the private SSL key (PEM format) used for both DTLS-SRTP (media) and WSS (signaling) connections. <b>Required</b>.
* @param strCA Path (relative or absolute) to the certificate authority used to sign the private and public keys. Used for both DTLS-SRTP (media) and WSS (signaling) connections. <b>Required</b>.
* @param bMutualAuth Whether to enable mutual authentication (apply to the signaling only).
* @retval <b>true</b> if no error; otherwise <b>false</b>.
*/
bool SCEngine::setSSLCertificates(const char* strPublicKey, const char* strPrivateKey, const char* strCA, bool bMutualAuth /*= false*/)
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

/**@ingroup _Group_CPP_Engine
* Sets the default prefered video size.
* @param strPrefVideoSize The prefered size. Accepted values:<br />
* "sqcif"(128x98)<br />
* "qcif"(176x144)<br />
* "qvga"(320x240)<br />
* "cif"(352x288)<br />
* "hvga"(480x320)<br />
* "vga"(640x480)<br />
* "4cif"(704x576)<br />
* "svga"(800x600)<br />
* "480p"(852x480)<br />
* "720p"(1280x720)<br />
* "16cif"(1408x1152)<br />
* "1080p"(1920x1080)<br />
* "2160p"(3840x2160)<br />
* @retval <b>true</b> if no error; otherwise <b>false</b>.
*/
bool SCEngine::setVideoPrefSize(const char* strPrefVideoSize)
{
    SC_ASSERT(strPrefVideoSize);
    int i;
    struct pref_video_size {
        const char* name;
        tmedia_pref_video_size_t size;
        size_t w;
        size_t h;
    };
    static const pref_video_size pref_video_sizes[] = {
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

    for (i = 0; i < pref_video_sizes_count; ++i) {
        if (tsk_striequals(pref_video_sizes[i].name, strPrefVideoSize)) {
            if (tmedia_defaults_set_pref_video_size(pref_video_sizes[i].size) == 0) {
                return true;
            }
        }
    }
    SC_DEBUG_ERROR("%s not valid as video size. Valid values: ...", strPrefVideoSize);
    return false;
}

/**@ingroup _Group_CPP_Engine
* Sets the default video frame rate.
* @param fps The frame rate (frames per second). Range: [1, 120].
* @retval <b>true</b> if no error; otherwise <b>false</b>. 
*/
bool SCEngine::setVideoFps(int fps)
{
    return ((tmedia_defaults_set_video_fps(fps) == 0));
}

/**@ingroup _Group_CPP_Engine
* Sets the maximum bandwidth (kbps) to use for outgoing video stream. <br />
* If congestion control is enabled then, the bandwidth will be updated based on the network conditions but these new values will never be higher than what you defined using this function.
* @param bandwwidthMax The new maximum bandwidth. Negative value means "compute best value".
* @retval <b>true</b> if no error; otherwise <b>false</b>. 
*/
bool SCEngine::setVideoBandwidthUpMax(int bandwwidthMax)
{
    return ((tmedia_defaults_set_bandwidth_video_upload_max(bandwwidthMax) == 0));
}

/**@ingroup _Group_CPP_Engine
* Sets the maximum bandwidth (kbps) to use for incoming video stream. <br />
* If congestion control is enabled then, the bandwidth will be updated based on the network conditions but these new values will never be higher than what you defined using this function.
* @param bandwwidthMax The new maximum bandwidth. Negative value means "compute best value".
* @retval <b>true</b> if no error; otherwise <b>false</b>.
*/
bool SCEngine::setVideoBandwidthDownMax(int bandwwidthMax)
{
    return ((tmedia_defaults_set_bandwidth_video_download_max(bandwwidthMax) == 0));
}

/**@ingroup _Group_CPP_Engine
* Sets the video type. Supported values: 1 (low, e.g. home video security systems), 2 (medium, e.g conference call) or 3 (high, e.g. basketball game). <br />
* Formula:
* @code
* [[video-max-upload-bandwidth (kbps) = ((video-width * video-height * video-fps * motion-rank * 0.07) / 1024)]]
* @endcode
* @param motionRank New motion rank value.
* @retval <b>true</b> if no error; otherwise <b>false</b>.
*/
bool SCEngine::setVideoMotionRank(int motionRank)
{
    return ((tmedia_defaults_set_video_motion_rank(motionRank) == 0));
}

/**@ingroup _Group_CPP_Engine
* Defines whether to adapt the bandwidth usage based on the congestion (packet loss/retransmission/delay). This mainly use non-standard algorithms and partially implements draft-alvestrand-rtcweb-congestion-03 and draft-alvestrand-rmcat-remb-01.
* @param congestionCtrl Enable/disable congestion control. Default: <b>false</b>.
* @retval <b>true</b> if no error; otherwise <b>false</b>.
*/
bool SCEngine::setVideoCongestionCtrlEnabled(bool congestionCtrl)
{
    return ((tmedia_defaults_set_congestion_ctrl_enabled(congestionCtrl ? tsk_true : tsk_false) == 0));
}

/**@ingroup _Group_CPP_Engine
* Sets the STUN/TURN server address. This server is used for NAT Traversal.
* @param host Hostname or IP address.
* @param port Port number. Range: [1024-65555].
* @retval <b>true</b> if no error; otherwise <b>false</b>.
*/
bool SCEngine::setNattStunServer(const char* host, unsigned short port /*= 3478*/)
{
    return ((tmedia_defaults_set_stun_server(host, port) == 0));
}

/**@ingroup _Group_CPP_Engine
* Sets the STUN/TURN credentials. These credentials are used for long-term authentication. Required for TURN and optional for STUN.
* @param username Username.
* @param password Pasword.
* @retval <b>true</b> if no error; otherwise <b>false</b>.
*/
bool SCEngine::setNattStunCredentials(const char* username, const char* password)
{
    return ((tmedia_defaults_set_stun_cred(username, password) == 0));
}

/**@ingroup _Group_CPP_Engine
* Defines whether to gather ICE reflexive candidates.
* @param enabled
* @retval <b>true</b> if no error; otherwise <b>false</b>.
*/
bool SCEngine::setNattIceStunEnabled(bool enabled)
{
    return ((tmedia_defaults_set_icestun_enabled(enabled ? tsk_true : tsk_false) == 0));
}

bool SCEngine::setNattIceTurnEnabled(bool enabled)
{
    return ((tmedia_defaults_set_iceturn_enabled(enabled ? tsk_true : tsk_false) == 0));
}
