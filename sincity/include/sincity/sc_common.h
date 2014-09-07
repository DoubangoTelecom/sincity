#ifndef SINCITY_COMMON_H
#define SINCITY_COMMON_H

#include "sc_config.h"

#define SC_CAT_(A, B) A ## B
#define SC_CAT(A, B) SC_CAT_(A, B)
#define SC_STRING_(A) #A
#define SC_STRING(A) SC_STRING_(A)

#if !defined(SC_SAFE_DELETE_CPP)
#	define SC_SAFE_DELETE_CPP(cpp_obj) if(cpp_obj) delete (cpp_obj), (cpp_obj) = NULL;
#endif

#if defined(NDEBUG)
#       define SC_ASSERT(x) (void)(x)
#else
#       define SC_ASSERT(x) assert(x)
#endif

#define kJsonContentType "application/json"

#define kSchemeWSS "wss"
#define kSchemeWS "ws"

typedef int32_t SCNetFd;
#define SCNetFd_IsValid(self)	((self) > 0)
#define kSCNetFdInvalid			-1

typedef enum SCDebugLevel_e
{
	SCDebugLevel_Info = 4,
	SCDebugLevel_Warn = 3,
	SCDebugLevel_Error = 2,
	SCDebugLevel_Fatal = 1
}
SCDebugLevel_t;

typedef enum SCSessionState_e
{
	SCSessionState_None,
	SCSessionState_Connecting,
	SCSessionState_Connected,
	SCSessionState_Terminated
}
SCSessionState_t;

typedef enum SCMediaType_e
{
	SCMediaType_None = 0x00,
	SCMediaType_Audio = (0x01<<0),
	SCMediaType_Video = (0x01<<1),
	SCMediaType_AudioVideo = (SCMediaType_Audio | SCMediaType_Video),

	SCMediaType_All = (SCMediaType_AudioVideo),
}
SCMediaType_t;

typedef enum SCSessionType_e
{
	SCSessionType_None,
	SCSessionType_Call
}
SCSessionType_t;

typedef enum SCNetTransporType_e
{
	SCNetTransporType_None = 0x00,
	SCNetTransporType_TCP = (0x01 << 0),
	SCNetTransporType_TLS = (0x01 << 1),
	SCNetTransporType_WS = ((0x01 << 2) | SCNetTransporType_TCP),
	SCNetTransporType_WSS = ((0x01 << 3) | SCNetTransporType_TLS),
	SCNetTransporType_HTTP = ((0x01 << 4) | SCNetTransporType_TCP),
	SCNetTransporType_HTTPS = ((0x01 << 5) | SCNetTransporType_TLS),
}
SCNetTransporType_t;

typedef enum SCWsActionType_e
{
	SCWsActionType_None = 0,
}
SCWsActionType_t;

typedef enum SCUrlHostType_e
{
	SCUrlHostType_None = 0,
	SCUrlHostType_Hostname,
	SCUrlHostType_IPv4,
	SCUrlHostType_IPv6
}
SCUrlHostType_t;

typedef enum SCUrlType_e
{
	SCUrlType_None = SCNetTransporType_None,
	SCUrlType_TCP = SCNetTransporType_TCP,
	SCUrlType_TLS = SCNetTransporType_TLS,
	SCUrlType_WS = SCNetTransporType_WS,
	SCUrlType_WSS = SCNetTransporType_WSS,
	SCUrlType_HTTP = SCNetTransporType_HTTP,
	SCUrlType_HTTPS = SCNetTransporType_HTTPS,
}
SCUrlType_t;

static bool SC_INLINE SCNetTransporType_isStream(SCNetTransporType_t eType)
{
	switch(eType)
	{
	case SCNetTransporType_TCP:
	case SCNetTransporType_TLS:
	case SCNetTransporType_WS:
	case SCNetTransporType_WSS:
		return true;
	default:
		return false;
	}
}

#define kSCMobuleNameNetTransport "NetTransport"
#define kSCMobuleNameWsTransport "WebSocketTransport"
#define kSCMobuleNameSignaling "Signaling"

#endif /* SINCITY_COMMON_H */
