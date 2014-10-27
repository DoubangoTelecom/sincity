#include "sincity/sc_api.h"

#include <assert.h>
#include <stdio.h>
#include <fcntl.h>

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

SCObjWrapper<SCSessionCall*>callSession;
SCObjWrapper<SCSignaling*>signalSession;
SCObjWrapper<SCSignalingCallEvent*>pendingOffer;
Json::Value jsonConfig;
bool connected = false;

#if !defined(MAX_PATH)
#	if defined(PATH_MAX)
#		define MAX_PATH PATH_MAX
#	else
#		define MAX_PATH 260
#	endif
#endif

static void printHelp();
static bool loadConfig();
static char config_file_path[MAX_PATH < 260 ? 260 : MAX_PATH] = { "./config.json" };

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
	char command[1024] = { 0 };
	char remoteId[25];

    printf("*******************************************************************\n"
           "Copyright (C) 2014 Doubango Telecom (VoIP division)\n"
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
		else if (strnicmp(command, "call", 4) == 0) {
			std::string strRemoteId = jsonConfig["remote_id"].asString();
			CHECK_CONNECTED();
			if (callSession) {
				SC_DEBUG_INFO("+++ already on call +++");
				continue;
			}
			if (sscanf(command, "call %24s", remoteId) != EOF && strlen(remoteId) > 0) {
				strRemoteId = std::string(remoteId);
			}
			SC_DEBUG_INFO("+++ call(%s) +++", strRemoteId.c_str());
			SC_ASSERT(callSession = SCSessionCall::newObj(signalSession));
			SC_ASSERT(callSession->call(SCMediaType_ScreenCast, strRemoteId));
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
                SC_ASSERT(callSession->acceptEvent(pendingOffer));
                pendingOffer = NULL;
            }
		}
    }

	if (callSession) {
		SC_ASSERT(callSession->hangup());
		callSession = NULL;
	}
	signalSession = NULL;

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
           "\"help\"         Prints this message\n"
		   "\"call [dest]\"  Makes call to \"dest\" (optional, default from config.json)\n"
		   "\"hangup\"       Terminates the active call\n"
		   "\"accept\"       Accepts the incoming call\n"
		   "\"reject\"       Rejects the incoming call\n"
		   "\"quit\"         Teminates the application\n"
           "--------------------------------------------------------------------------\n\n"
           );
}