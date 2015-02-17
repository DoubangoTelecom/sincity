#include "sincity/sc_api.h"

#include <assert.h>
#include <stdio.h>
#if !SC_UNDER_WINDOWS_CE
#include <fcntl.h>
#endif /* !SC_UNDER_WINDOWS_CE */

#if SC_UNDER_WINDOWS
#include <windows.h>
#endif /* SC_UNDER_WINDOWS */

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
#endif /* _MSC_VER */

static SCObjWrapper<SCSessionCall*>callSession;
static SCObjWrapper<SCSignaling*>signalSession;
static SCObjWrapper<SCSignalingCallEvent*>pendingOffer;
static SCObjWrapper<SCThread*>threadConsoleReader;
static Json::Value jsonConfig;
static SCVideoDisplay displayVideoLocal = NULL;
static SCVideoDisplay displayVideoRemote = NULL;
static SCVideoDisplay displayScreenCastLocal = NULL;
static SCVideoDisplay displayScreenCastRemote = NULL;
static bool connected = false;

#if SC_UNDER_WINDOWS
#define WM_SC_ATTACH_DISPLAYS			(WM_USER + 201)
static DWORD mainThreadId = 0;
#define DEFAULT_VIDEO_REMOTE_WINDOW_NAME	L"Remote video window (Decoded RTP)" // Remote window is where the decoded video frames are displayed
#define DEFAULT_VIDEO_LOCAL_WINDOW_NAME		L"Local video window (Preview)" // Local window is where the encoded video frames are displayed before sending (preview, PIP mode).
static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif /* SC_UNDER_WINDOWS */

#if !defined(MAX_PATH)
#	if defined(PATH_MAX)
#		define MAX_PATH PATH_MAX
#	else
#		define MAX_PATH 260
#	endif
#endif /* MAX_PATH */

static void printHelp();
static bool loadConfig();
static char config_file_path[MAX_PATH < 260 ? 260 : MAX_PATH] = { "./config.json" };

static void* SC_STDCALL consoleReaderProc(void *arg);
static bool attachDisplays();
static SCVideoDisplay getDisplay(bool bRemote, bool bScreenCast = false);

// Media type to use for "chat" command
static const SCMediaType_t kDefaultMediaType = ((SCMediaType_t)(SCMediaType_Audio | SCMediaType_Video)); // Media type to use for "chat" command

class SCSessionCallIceCallbackDummy : public SCSessionCallIceCallback
{
public:
	SCSessionCallIceCallbackDummy() {
	}
	virtual ~SCSessionCallIceCallbackDummy() {
		SC_DEBUG_INFO("*** SCSessionCallIceCallbackDummy destroyed ***");
	}
	virtual SC_INLINE const char* getObjectId() {
		return "SCSessionCallIceCallbackDummy";
	}
	virtual bool onStateChanged(SCObjWrapper<SCSessionCall*> oCall) {
		if (oCall->getIceState() == SCIceState_Connected) {
			oCall->sessionMgrStart();
		}
		return true;
	}
	static SCObjWrapper<SCSessionCallIceCallback*> newObj() {
		return new SCSessionCallIceCallbackDummy();
	}
};


class SCSignalingCallbackDummy : public SCSignalingCallback
{
protected:
    SCSignalingCallbackDummy() {

    }
public:
    virtual ~SCSignalingCallbackDummy() {

    }

    virtual bool onEventNet(SCObjWrapper<SCSignalingEvent*>& e) {
        //!\Deadlock issue: You must not call any function from 'SCSignaling' class unless you fork a new thread.
        switch (e->getType()) {
        case SCSignalingEventType_NetReady: {
            connected = true;
            SC_DEBUG_INFO("***Signaling module connected ***");
            break;
        }
        case SCSignalingEventType_NetDisconnected:
        case SCSignalingEventType_NetError: {
            connected = false;
            SC_DEBUG_INFO("***Signaling module disconnected ***");
            break;
        }
        case SCSignalingEventType_NetData: {
            SC_DEBUG_INFO("***Signaling module passthrough DATA:%.*s ***", e->getDataSize(), (const char*)e->getDataPtr());
            break;
        }
        }

        return true;
    }
    virtual SC_INLINE const char* getObjectId() {
        return "SCSignalingCallbackDummy";
    }
    virtual bool onEventCall(SCObjWrapper<SCSignalingCallEvent*>& e) {
        //!\Deadlock issue: You must not call any function from 'SCSignaling' class unless you fork a new thread.
        if (callSession) {
            if (callSession->getCallId() != e->getCallId()) {
                SC_DEBUG_ERROR("Call id mismatch: '%s'<>'%s'", callSession->getCallId().c_str(), e->getCallId().c_str());
                return SCSessionCall::rejectEvent(signalSession, e);
            }
            bool ret = callSession->acceptEvent(e);
            if (e->getType() == "hangup") {
                callSession = NULL;
                SC_DEBUG_INFO("+++Call ended +++");
            }
            return ret;
        }
        else {
            if (e->getType() == "offer") {
                if (callSession || pendingOffer) { // already in call?
                    return SCSessionCall::rejectEvent(signalSession, e);
                }
                pendingOffer = e;
                SC_DEBUG_INFO("+++Incoming call: 'accept'/'reject'? +++");
            }
            if (e->getType() == "hangup") {
                if (pendingOffer && pendingOffer->getCallId() == e->getCallId()) {
                    pendingOffer = NULL;
                    SC_DEBUG_INFO("+++ pending call cancelled +++");
                }
            }

            // Silently ignore any other event type
        }

        return true;
    }

    static SCObjWrapper<SCSignalingCallback*> newObj() {
        return new SCSignalingCallbackDummy();
    }
};

#if defined(_WIN32_WCE) || defined(UNDER_CE)
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char* argv[])
#endif
{
    SC_ASSERT(threadConsoleReader = SCThread::newObj(consoleReaderProc));

    printf("*******************************************************************\n"
           "Copyright (C) 2014-2015 Doubango Telecom (VoIP division)\n"
           "PRODUCT: SINCITY\n"
           "HOME PAGE: <void>\n"
           "CODE SOURCE: <void>\n"
           "LICENCE: <void>\n"
           "VERSION: %s\n"
           "'quit' to quit the application.\n"
           "*******************************************************************\n\n"
           , SC_VERSION_STRING);

    /* load configuration */
    SC_ASSERT(loadConfig());

    SC_ASSERT(SCEngine::init(jsonConfig["local_id"].asCString()));
    SC_ASSERT(SCEngine::setDebugLevel(jsonConfig["debug_level"].isNumeric() ? (SCDebugLevel_t)jsonConfig["debug_level"].asInt() : SCDebugLevel_Info));

    static const char* __entries[] = {
        "debug_level",
        "ssl_file_pub", "ssl_file_priv", "ssl_file_ca", "connection_url", "local_id", "remote_id",
        "video_pref_size", "video_fps", "video_bandwidth_up_max", "video_bandwidth_down_max", "video_motion_rank", "video_congestion_ctrl_enabled",
        "video_jb_enabled", "video_zeroartifacts_enabled", "video_avpf_tail",
        "audio_echo_supp_enabled", "audio_echo_tail",
        "natt_ice_servers", "natt_ice_stun_enabled", "natt_ice_turn_enabled"
    };
    for (size_t i = 0; i < sizeof(__entries)/sizeof(__entries[0]); ++i) {
        SC_DEBUG_INFO_EX("CONFIG", "%s: %s", __entries[i], jsonConfig[__entries[i]].toStyledString().c_str());
    }

    SC_ASSERT(SCEngine::setSSLCertificates(
                  jsonConfig["ssl_file_pub"].isString() ? jsonConfig["ssl_file_pub"].asCString() : "SSL_Pub.pem",
                  jsonConfig["ssl_file_priv"].isString() ? jsonConfig["ssl_file_priv"].asCString() : "SSL_Priv.pem",
                  jsonConfig["ssl_file_ca"].isString() ? jsonConfig["ssl_file_ca"].asCString() : "SSL_CA.pem"
              ));
    if (jsonConfig["video_pref_size"].isString()) {
        SC_ASSERT(SCEngine::setVideoPrefSize(jsonConfig["video_pref_size"].asCString()));
    }
    if (jsonConfig["video_fps"].isNumeric()) {
        SC_ASSERT(SCEngine::setVideoFps(jsonConfig["video_fps"].asInt()));
    }
    if (jsonConfig["video_bandwidth_up_max"].isNumeric()) {
        SC_ASSERT(SCEngine::setVideoBandwidthUpMax(jsonConfig["video_bandwidth_up_max"].asInt()));
    }
    if (jsonConfig["video_bandwidth_down_max"].isNumeric()) {
        SC_ASSERT(SCEngine::setVideoBandwidthDownMax(jsonConfig["video_bandwidth_down_max"].asInt()));
    }
    if (jsonConfig["video_motion_rank"].isNumeric()) {
        SC_ASSERT(SCEngine::setVideoMotionRank(jsonConfig["video_motion_rank"].asInt()));
    }
    if (jsonConfig["video_congestion_ctrl_enabled"].isBool()) {
        SC_ASSERT(SCEngine::setVideoCongestionCtrlEnabled(jsonConfig["video_congestion_ctrl_enabled"].asBool()));
    }
    if (jsonConfig["video_jb_enabled"].isBool()) {
        SC_ASSERT(SCEngine::setVideoJbEnabled(jsonConfig["video_jb_enabled"].asBool()));
    }
    if (jsonConfig["video_zeroartifacts_enabled"].isBool()) {
        SC_ASSERT(SCEngine::setVideoZeroArtifactsEnabled(jsonConfig["video_zeroartifacts_enabled"].asBool()));
    }
    if (jsonConfig["video_avpf_tail"].isString()) {
        char min[24], max[24];
        SC_ASSERT(sscanf(jsonConfig["video_avpf_tail"].asCString(), "%23s %23s", min, max) != EOF);
        SC_ASSERT(SCEngine::setVideoAvpfTail(atoi(min), atoi(max)));
    }
    if (jsonConfig["audio_echo_supp_enabled"].isBool()) {
        SC_ASSERT(SCEngine::setAudioEchoSuppEnabled(jsonConfig["audio_echo_supp_enabled"].asBool()));
    }
    if (jsonConfig["audio_echo_tail"].isNumeric()) {
        SC_ASSERT(SCEngine::setAudioEchoTail(jsonConfig["audio_echo_tail"].asInt()));
    }

#if 0 // Deprecated in release 1.A
    if (jsonConfig["natt_stun_server_host"].isString() && jsonConfig["natt_stun_server_port"].isNumeric()) {
        SC_ASSERT(SCEngine::setNattStunServer(jsonConfig["natt_stun_server_host"].asCString(), jsonConfig["natt_stun_server_port"].asInt()));
    }
    SC_ASSERT(SCEngine::setNattStunCredentials(
                  jsonConfig["natt_stun_username"].isString() ? jsonConfig["natt_stun_username"].asCString() : NULL,
                  jsonConfig["natt_stun_password"].isString() ? jsonConfig["natt_stun_password"].asCString() : NULL
              ));
#else
    if (jsonConfig["natt_ice_servers"].isArray() && jsonConfig["natt_ice_servers"].size() > 0) {
        for (Json::ArrayIndex IceServerIndex = 0; IceServerIndex < jsonConfig["natt_ice_servers"].size(); ++IceServerIndex) {
            SC_ASSERT(SCEngine::addNattIceServer(
                          jsonConfig["natt_ice_servers"][IceServerIndex]["protocol"].asCString(),
                          jsonConfig["natt_ice_servers"][IceServerIndex]["host"].asCString(),
                          jsonConfig["natt_ice_servers"][IceServerIndex]["port"].asUInt(),
                          jsonConfig["natt_ice_servers"][IceServerIndex]["enable_turn"].asBool(),
                          jsonConfig["natt_ice_servers"][IceServerIndex]["enable_stun"].asBool(),
                          jsonConfig["natt_ice_servers"][IceServerIndex]["login"].asCString(),
                          jsonConfig["natt_ice_servers"][IceServerIndex]["password"].asCString()
                      ));
        }
    }
#endif

    if (jsonConfig["natt_ice_stun_enabled"].isBool()) {
        SC_ASSERT(SCEngine::setNattIceStunEnabled(jsonConfig["natt_ice_stun_enabled"].asBool()));
    }
    if (jsonConfig["natt_ice_turn_enabled"].isBool()) {
        SC_ASSERT(SCEngine::setNattIceTurnEnabled(jsonConfig["natt_ice_turn_enabled"].asBool()));
    }

    /* connect */
    signalSession = SCSignaling::newObj(jsonConfig["connection_url"].asCString());
    SC_ASSERT(signalSession);

    SC_ASSERT(signalSession->setCallback(SCSignalingCallbackDummy::newObj()));
    SC_ASSERT(signalSession->connect());

    /* print help */
    printHelp();

#if SC_UNDER_WINDOWS
    mainThreadId = GetCurrentThreadId();
    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message == WM_QUIT) {
            break;
        }
        else if (msg.message == WM_SC_ATTACH_DISPLAYS) {
            SC_DEBUG_INFO("Catching 'WM_SC_ATTACH_DISPLAYS' windows event");
            SC_ASSERT(attachDisplays());
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
#else
    threadConsoleReader->join();
#endif

    threadConsoleReader = NULL;

    return 0;
}

static bool loadConfig()
{
    if (!SCUtils::fileExists(config_file_path)) {
        snprintf(config_file_path, sizeof(config_file_path), "%s/%s", SCUtils::currentDirectoryPath(), "config.json");
        if (!SCUtils::fileExists(config_file_path)) {
            static const char* config_file_paths[] = { "../../config.json", "../config.json" };
            for (size_t i = 0; i < sizeof(config_file_paths)/sizeof(config_file_paths[0]); ++i) {
                if (SCUtils::fileExists(config_file_paths[i])) {
                    memcpy(config_file_path, config_file_paths[i], strlen(config_file_paths[i]));
                    config_file_path[strlen(config_file_paths[i])] = '\0';
                    break;
                }
            }
        }
    }

    FILE* p_file = fopen(config_file_path, "rb");
    if (!p_file) {
        SC_DEBUG_ERROR("Failed to open file at %s", config_file_path);
        return false;
    }
    fseek(p_file, 0, SEEK_END);
    long fsize = ftell(p_file);
    fseek(p_file, 0, SEEK_SET);

    char *p_buffer = new char[fsize + 1];
    SC_ASSERT(p_buffer);
    p_buffer[fsize] = 0;
    SC_ASSERT(fread(p_buffer, 1, fsize, p_file) == fsize);
    std::string jsonText(p_buffer);
    fclose(p_file);
    delete[] p_buffer;

    Json::Reader reader;
    SC_ASSERT(reader.parse(jsonText, jsonConfig, false));

    return true;
}

static void printHelp()
{
    printf("----------------------------------------------------------------------------\n"
           "                               COMMANDS                                     \n"
           "----------------------------------------------------------------------------\n"
           "\"help\"              Prints this message\n"
           "\"chat [dest]\"       Audio/Video call to \"dest\" (opt., default from config.json)\n"
           "\"audio [dest]\"      Audio call to \"dest\" (opt., default from config.json)\n"
           "\"video [dest]\"      Video call to \"dest\" (opt., default from config.json)\n"
           "\"screencast [dest]\" Share screen with \"dest\" (opt., default from config.json)\n"
           "\"hangup\"            Terminates the active call\n"
           "\"accept\"            Accepts the incoming call\n"
           "\"reject\"            Rejects the incoming call\n"
           "\"quit\"              Teminates the application\n"
           "--------------------------------------------------------------------------\n\n"
          );
}

static void* SC_STDCALL consoleReaderProc(void *arg)
{
    char command[1024] = { 0 };
    char remoteId[25];

    SC_DEBUG_INFO("consoleReaderProc ---ENTER---");

    while (fgets(command, sizeof(command), stdin) != NULL) {
#define CHECK_CONNECTED() if (!connected){ SC_DEBUG_INFO("+++ not connected yet +++"); continue; }
        if (strnicmp(command, "quit", 4) == 0) {
            SC_DEBUG_INFO("+++ quit() +++");
            break;
        }
        else if (strnicmp(command, "help", 4) == 0) {
            SC_DEBUG_INFO("+++ help() +++");
            printHelp();
        }
        else if (strnicmp(command, "chat", 4) == 0 || strnicmp(command, "audio", 5) == 0 || strnicmp(command, "video", 5) == 0 || strnicmp(command, "screencast", 10) == 0 || strnicmp(command, "call", 4) == 0) {
            std::string strRemoteId = jsonConfig["remote_id"].asString();
            CHECK_CONNECTED();
            if (callSession) {
                SC_DEBUG_INFO("+++ already on call +++");
                continue;
            }
            if (sscanf(command, "%*s %24s", remoteId) > 0 && strlen(remoteId) > 0) {
                strRemoteId = std::string(remoteId);
            }
            SCMediaType_t mediaType = kDefaultMediaType;
            if (strnicmp(command, "audio", 5) == 0) {
                mediaType = SCMediaType_Audio;
            }
            else if (strnicmp(command, "video", 5) == 0) {
                mediaType = SCMediaType_Video;
            }
            else if (strnicmp(command, "screencast", 10) == 0 || strnicmp(command, "call", 4) == 0) {
                mediaType = SCMediaType_ScreenCast;    // "call == screencast" -> backward compatibility
            }
            SC_DEBUG_INFO("+++ call('%s',%d) +++", strRemoteId.c_str(), mediaType);
            SC_ASSERT(callSession = SCSessionCall::newObj(signalSession));
			SC_ASSERT(callSession->setIceCallback(SCSessionCallIceCallbackDummy::newObj()));
            SC_ASSERT(callSession->call(mediaType, strRemoteId));
            SC_ASSERT(attachDisplays());
        }
        else if (strnicmp(command, "hangup", 6) == 0 || strnicmp(command, "reject", 6) == 0) {
            CHECK_CONNECTED();
            if (callSession) {
                SC_DEBUG_INFO("+++ hangup() +++");
                SC_ASSERT(callSession->hangup());
                callSession = NULL;
            }
            else if (pendingOffer) {
                SC_DEBUG_INFO("+++ reject() +++");
                SC_ASSERT(SCSessionCall::rejectEvent(signalSession, pendingOffer));
                pendingOffer = NULL;
            }
        }
        else if (strnicmp(command, "accept", 6) == 0) {
            CHECK_CONNECTED();
            if (!callSession && pendingOffer) {
                SC_DEBUG_INFO("+++ accept() +++");
                SC_ASSERT(callSession = SCSessionCall::newObj(signalSession, pendingOffer));
				SC_ASSERT(callSession->setIceCallback(SCSessionCallIceCallbackDummy::newObj()));
                SC_ASSERT(callSession->acceptEvent(pendingOffer));
                SC_ASSERT(attachDisplays());
                pendingOffer = NULL;
            }
        }
    }

    if (callSession) {
        SC_ASSERT(callSession->hangup());
        callSession = NULL;
    }
    signalSession = NULL;

    SC_DEBUG_INFO("consoleReaderProc ---EXIT---");

#if SC_UNDER_WINDOWS
    PostThreadMessage(mainThreadId, WM_QUIT, NULL, NULL);
#endif /* SC_UNDER_WINDOWS */

    return NULL;
}

static bool attachDisplays()
{
#if SC_UNDER_WINDOWS
    if ((mainThreadId != GetCurrentThreadId())) {
        SC_DEBUG_INFO("attachDisplays deferred because we are not on the main thread.... %lu<>%lu", mainThreadId, GetCurrentThreadId());
        PostThreadMessage(mainThreadId, WM_SC_ATTACH_DISPLAYS, NULL, NULL);
        return true;
    }
#endif /* SC_UNDER_WINDOWS */

    if (callSession) {
#if 1
        if ((callSession->getMediaType() & SCMediaType_ScreenCast)) {
            SC_ASSERT(callSession->setVideoDisplays(SCMediaType_ScreenCast, getDisplay(false/*remote?*/, true/*screencast?*/), getDisplay(true/*remote?*/, true/*screencast?*/)));
        }
        if ((callSession->getMediaType() & SCMediaType_Video)) {
            SC_ASSERT(callSession->setVideoDisplays(SCMediaType_Video, getDisplay(false/*remote?*/, false/*screencast?*/), getDisplay(true/*remote?*/, false/*screencast?*/)));
        }
#endif
    }
    return true;
}

static SCVideoDisplay getDisplay(bool bRemote, bool bScreenCast /*= false*/)
{
#if SC_UNDER_WINDOWS
#if !defined(WS_OVERLAPPEDWINDOW)
#define WS_OVERLAPPEDWINDOW (WS_OVERLAPPED     | \
                             WS_CAPTION        | \
                             WS_SYSMENU        | \
                             WS_THICKFRAME     | \
                             WS_MINIMIZEBOX    | \
                             WS_MAXIMIZEBOX)
#endif /* WS_OVERLAPPEDWINDOW */
    HWND* pHWND = bRemote ? (bScreenCast ? &displayScreenCastRemote : &displayVideoRemote) : (bScreenCast ? &displayScreenCastLocal : &displayVideoLocal);
    if (!*pHWND) {
        WNDCLASS wc = {0};

        wc.lpfnWndProc   = WindowProc;
        wc.hInstance     = GetModuleHandle(NULL);
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wc.lpszClassName =  L"MFCapture Window Class";
        /*GOTHAM_ASSERT*/(RegisterClass(&wc));

        SC_ASSERT(*pHWND = ::CreateWindow(
                               wc.lpszClassName,
                               bRemote ? DEFAULT_VIDEO_REMOTE_WINDOW_NAME : DEFAULT_VIDEO_LOCAL_WINDOW_NAME,
                               WS_OVERLAPPEDWINDOW,
                               CW_USEDEFAULT,
                               CW_USEDEFAULT,
                               bRemote ? CW_USEDEFAULT : 352,
                               bRemote ? CW_USEDEFAULT : 288,
                               NULL,
                               NULL,
                               GetModuleHandle(NULL),
                               NULL));

        ::SetWindowText(*pHWND, bRemote ? DEFAULT_VIDEO_REMOTE_WINDOW_NAME : DEFAULT_VIDEO_LOCAL_WINDOW_NAME);
#if SC_UNDER_WINDOWS_CE
        ::ShowWindow(*pHWND, SW_SHOWNORMAL);
#else
        ::ShowWindow(*pHWND, SW_SHOWDEFAULT);
#endif /* SC_UNDER_WINDOWS_CE */

        ::UpdateWindow(*pHWND);
    }
    return *pHWND;
#else
    return NULL;
#endif /* SC_UNDER_WINDOWS */
}

#if SC_UNDER_WINDOWS

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_DESTROY:
    case WM_CLOSE: {
        if (displayVideoLocal == hwnd) {
            displayVideoLocal = NULL;
        }
        else if (displayVideoRemote == hwnd) {
            displayVideoRemote = NULL;
        }
        else if (displayScreenCastLocal == hwnd) {
            displayScreenCastLocal = NULL;
        }
        else if (displayScreenCastRemote == hwnd) {
            displayScreenCastRemote = NULL;
        }
        break;
    }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

#endif /* SC_UNDER_WINDOWS */