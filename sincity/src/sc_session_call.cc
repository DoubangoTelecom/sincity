#include "sincity/sc_session_call.h"

#include "tinymedia.h"

static tmedia_type_t _mediaTypeToNative(SCMediaType_t mediaType)
{
	tmedia_type_t type = tmedia_none;
	if (mediaType & SCMediaType_Audio) type = (tmedia_type_t)(type | tmedia_audio);
	if (mediaType & SCMediaType_Video) type = (tmedia_type_t)(type | tmedia_video);
	// if (mediaType & SCMediaType_ScreenCast) type = (tmedia_type_t)(type | tmedia_bfcp_video); // FIXME
	return type;
}

SCSessionCall::SCSessionCall(std::string strUserId, SCObjWrapper<SCSignaling*> oSignaling)
    : SCSession(SCSessionType_Call, strUserId, oSignaling)
	, m_eMediaType(SCMediaType_None)
	, m_pIceCtxVideo(NULL)
	, m_pIceCtxAudio(NULL)
	
	, m_pSessionMgr(NULL)
	
	, m_pSdpLocal(NULL)
	, m_pSdpRemote(NULL)
{

}

SCSessionCall::~SCSessionCall()
{
	TSK_OBJECT_SAFE_FREE(m_pIceCtxVideo);
	TSK_OBJECT_SAFE_FREE(m_pIceCtxAudio);
	
	TSK_OBJECT_SAFE_FREE(m_pSessionMgr);
	
	TSK_OBJECT_SAFE_FREE(m_pSdpLocal);
	TSK_OBJECT_SAFE_FREE(m_pSdpRemote);
}

bool SCSessionCall::call(SCMediaType_t eMediaType, std::string srtDestUserId)
{
	if (eMediaType != SCMediaType_ScreenCast)
	{
		SC_DEBUG_ERROR("%d not a valid media type", eMediaType);
		return false;
	}

	if (!m_oSignaling->isReady())
	{
		SC_DEBUG_ERROR("Signaling layer not ready yet");
		return false;
	}

	// Create session manager if not already done
	if (m_pSessionMgr)
	{
		SC_DEBUG_ERROR("There is already an active call");
		return false;
	}
	
	// Update mediaType
	m_eMediaType = eMediaType;

	// Create local offer
	if (!createLocalOffer())
	{
		return false;
	}

	return true;
}

bool SCSessionCall::hangup()
{
	return false;
}

bool SCSessionCall::createSessionMgr()
{
	TSK_OBJECT_SAFE_FREE(m_pSessionMgr);

	TSK_OBJECT_SAFE_FREE(m_pIceCtxVideo);
	TSK_OBJECT_SAFE_FREE(m_pIceCtxAudio);

	TSK_OBJECT_SAFE_FREE(m_pSdpLocal);
	TSK_OBJECT_SAFE_FREE(m_pSdpRemote);

	m_pSessionMgr = tmedia_session_mgr_create(_mediaTypeToNative(m_eMediaType),
            "0.0.0.0", tsk_false/*IPv6*/, tsk_true/* offerer */);
	if (!m_pSessionMgr)
	{
		SC_DEBUG_ERROR("Failed to create media session manager");
		return false;
	}
	return true;
}

bool SCSessionCall::createLocalOffer()
{
	if (!m_pSessionMgr)
	{
		if (!createSessionMgr())
		{
			return false;
		}
	}
	else
	{
		// update media type
		if (tmedia_session_mgr_set_media_type(m_pSessionMgr, _mediaTypeToNative(m_eMediaType)) != 0)
		{
			SC_DEBUG_ERROR("Failed to update media type %d->%d", m_pSessionMgr->type, _mediaTypeToNative(m_eMediaType));
			return false;
		}
	}

	const tsdp_message_t* sdp_lo = tmedia_session_mgr_get_lo(m_pSessionMgr);
	if (!sdp_lo)
	{
		SC_DEBUG_ERROR("Cannot get local offer");
		return false;
	}
#if 1 // FIXME
	char* sdpStr = tsdp_message_tostring(sdp_lo);
	if (!*sdpStr)
	{
		SC_DEBUG_ERROR("Cannot serialize local offer");
		return false;
	}
	SC_DEBUG_INFO("local offer: %s", sdpStr);
	TSK_FREE(sdpStr);
#endif
	
	 TSK_OBJECT_SAFE_FREE(m_pSdpLocal);
	 m_pSdpLocal = (tsdp_message_t*)tsk_object_ref((tsk_object_t*)sdp_lo);
	 // mReadyState = (mSdpLocal && tmedia_session_mgr_get_ro(mSessionMgr)) ? ReadyStateActive : ReadyStateOpening;

	 return true;
}

SCObjWrapper<SCSessionCall*> SCSessionCall::newObj(std::string strUserId, SCObjWrapper<SCSignaling*> oSignaling)
{
	if (strUserId.empty() || !oSignaling)
	{
		SC_DEBUG_ERROR("Invalid argument");
		return NULL;
	}

    SCObjWrapper<SCSessionCall*> oCall = new SCSessionCall(strUserId, oSignaling);

    return oCall;
}
