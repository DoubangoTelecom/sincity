#include "sincity/sc_session_call.h"
#include "sincity/sc_engine.h"
#include "sincity/sc_utils.h"
#include "sincity/jsoncpp/sc_json.h"

#include "tinymedia.h"

#include <assert.h>

static tmedia_type_t _mediaTypeToNative(SCMediaType_t mediaType)
{
    tmedia_type_t type = tmedia_none;
    if (mediaType & SCMediaType_Audio) {
        type = (tmedia_type_t)(type | tmedia_audio);
    }
    if (mediaType & SCMediaType_Video) {
        type = (tmedia_type_t)(type | tmedia_video);
    }
    // if (mediaType & SCMediaType_ScreenCast) type = (tmedia_type_t)(type | tmedia_bfcp_video); // FIXME
    return type;
}

static SCMediaType_t _mediaTypeFromNative(tmedia_type_t mediaType)
{
    SCMediaType_t type = SCMediaType_None;
    if (mediaType & tmedia_audio) {
        type = (SCMediaType_t)(type | SCMediaType_Audio);
    }
    if (mediaType & tmedia_video) {
        type = (SCMediaType_t)(type | SCMediaType_Video);
    }
    // if (mediaType & SCMediaType_ScreenCast) type = (tmedia_type_t)(type | tmedia_bfcp_video); // FIXME
    return type;
}

SCSessionCall::SCSessionCall(SCObjWrapper<SCSignaling*> oSignaling, std::string strCallId /*= ""*/)
    : SCSession(SCSessionType_Call, oSignaling)
    , m_eMediaType(SCMediaType_None)

    , m_pIceCtxVideo(NULL)
    , m_pIceCtxAudio(NULL)

    , m_pSessionMgr(NULL)

	, m_strCallId(strCallId)

    , m_oMutex(new SCMutex())
{
	if (m_strCallId.empty())
	{
		m_strCallId = SCUtils::randomString();
	}
}

SCSessionCall::~SCSessionCall()
{
   cleanup();
}

void SCSessionCall::lock()
{
    m_oMutex->lock();
}

void SCSessionCall::unlock()
{
    m_oMutex->unlock();
}

bool SCSessionCall::call(SCMediaType_t eMediaType, std::string strDestUserId)
{
    SCAutoLock<SCSessionCall> autoLock(this);

    if (eMediaType != SCMediaType_ScreenCast) {
        SC_DEBUG_ERROR("%d not a valid media type", eMediaType);
        return false;
    }

    if (!m_oSignaling->isReady()) {
        SC_DEBUG_ERROR("Signaling layer not ready yet");
        return false;
    }

    // Create session manager if not already done
    if (m_pSessionMgr) {
        SC_DEBUG_ERROR("There is already an active call");
        return false;
    }

	// Set local SDP type
	m_strLocalSdpType = "offer";

    // Update mediaType
    m_eMediaType = eMediaType;

    // Update destination id
    m_strDestUserId = strDestUserId;

    // Create local offer
    if (!createLocalOffer()) {
        return false;
    }

    return true;
}

bool SCSessionCall::handEvent(SCObjWrapper<SCSignalingCallEvent*>& e)
{
    SCAutoLock<SCSessionCall> autoLock(this);

    if (e->getCallId().compare(m_strCallId) != 0) {
        SC_DEBUG_ERROR("CallId mismatch: '%s'<>'%s'", e->getCallId().c_str(), m_strCallId.c_str());
        return false;
    }

    if (e->getType() == "hangup") {
		return cleanup();
    }

    if (e->getType() == "ack") {
        SC_DEBUG_ERROR("Not implemented yet");
        return false;
    }

    bool bRet = true;
    tsdp_message_t* pSdp = NULL;

    if (e->getType() == "offer" || e->getType() == "answer" || e->getType() == "pranswer") {
        int iRet = 0;
        tmedia_ro_type_t ro_type;
		SCMediaType_t newMediaType;

        // Parse SDP
        if (!(pSdp = tsdp_message_parse(e->getSdp().c_str(), e->getSdp().length()))) {
            SC_DEBUG_ERROR("Failed to parse SDP: %s", e->getSdp().c_str());
            bRet = false;
            goto bail;
        }

        if (!iceIsEnabled(pSdp)) {
            SC_DEBUG_ERROR("ICE is required. SDP='%s'", e->getSdp().c_str());
            bRet = false;
            goto bail;
        }

        ro_type = (e->getType() == "pranswer")
                  ? tmedia_ro_type_provisional
                  : (e->getType() == "answer" ? tmedia_ro_type_answer : tmedia_ro_type_offer);

		newMediaType = _mediaTypeFromNative(tmedia_type_from_sdp(pSdp));

		m_eMediaType = (SCMediaType_t)(newMediaType & SCMediaType_Video); // FIXME: remove all media types except "Video"
        if (!createLocalOffer(pSdp, ro_type)) {
            bRet = false;
            goto bail;
        }

		// Start session manager if ICE done and not already started
		if (iceIsDone() && e->getType() != "offer")
		{
			if (!iceProcessRo(pSdp, (ro_type == tmedia_ro_type_offer))) {
				bRet = false;
				goto bail;
			}
		}
    }

bail:
    TSK_OBJECT_SAFE_FREE(pSdp);
    return bRet;
}

bool SCSessionCall::hangup()
{
	SCAutoLock<SCSessionCall> autoLock(this);
	
	Json::Value message;

	message["type"] = "hangup";
    message["cid"] = m_strCallId;
	message["tid"] = SCUtils::randomString();
    message["from"] = SCEngine::s_strCredUserId;
    message["to"] = m_strDestUserId;

    Json::StyledWriter writer;
    std::string output = writer.write(message);
    if (output.empty()) {
        SC_DEBUG_ERROR("Failed serialize JSON content");
        return false;
    }

    if (!m_oSignaling->sendData(output.c_str(), output.length()))
	{
		return false;
	}

	cleanup();

	return true;
}

bool SCSessionCall::cleanup()
{
	SCAutoLock<SCSessionCall> autoLock(this);

	TSK_OBJECT_SAFE_FREE(m_pSessionMgr);

    TSK_OBJECT_SAFE_FREE(m_pIceCtxVideo);
    TSK_OBJECT_SAFE_FREE(m_pIceCtxAudio);

	m_strLocalSdpType = "";

	return true;
}

bool SCSessionCall::createSessionMgr()
{
    SCAutoLock<SCSessionCall> autoLock(this);

    TSK_OBJECT_SAFE_FREE(m_pSessionMgr);

    TSK_OBJECT_SAFE_FREE(m_pIceCtxVideo);
    TSK_OBJECT_SAFE_FREE(m_pIceCtxAudio);

    // Create ICE contexts
    if (!iceCreateCtx()) {
        return false;
    }

    int iRet;
    tnet_ip_t bestsource;
    if ((iRet = tnet_getbestsource("stun.l.google.com", 19302, tnet_socket_type_udp_ipv4, &bestsource))) {
        SC_DEBUG_ERROR("Failed to get best source [%d]", iRet);
        memcpy(bestsource, "0.0.0.0", 7);
    }

    // Create media session manager
    m_pSessionMgr = tmedia_session_mgr_create(_mediaTypeToNative(m_eMediaType),
                    bestsource, tsk_false/*IPv6*/, tsk_true/* offerer */);
    if (!m_pSessionMgr) {
        SC_DEBUG_ERROR("Failed to create media session manager");
        return false;
    }

    // Set ICE contexts
    if (tmedia_session_mgr_set_ice_ctx(m_pSessionMgr, m_pIceCtxAudio, m_pIceCtxVideo) != 0) {
        SC_DEBUG_ERROR("Failed to set ICE contexts");
        return false;
    }

    return true;
}

bool SCSessionCall::createLocalOffer(const struct tsdp_message_s* pc_Ro /*= NULL*/, enum tmedia_ro_type_e eRoType /*= 0*/)
{
    SCAutoLock<SCSessionCall> autoLock(this);

    if (!m_pSessionMgr) {
        if (!createSessionMgr()) {
            return false;
        }
    }
    else {
        // update media type
        if (tmedia_session_mgr_set_media_type(m_pSessionMgr, _mediaTypeToNative(m_eMediaType)) != 0) {
            SC_DEBUG_ERROR("Failed to update media type %d->%d", m_pSessionMgr->type, _mediaTypeToNative(m_eMediaType));
            return false;
        }
    }

	if (pc_Ro) {
		if (tmedia_session_mgr_set_ro(m_pSessionMgr, pc_Ro, eRoType) != 0) {
			SC_DEBUG_ERROR("Failed to set remote offer");
            return false;
		}
		if (!iceProcessRo(pc_Ro, (eRoType == tmedia_ro_type_offer)))
		{
			return false;
		}
	}

	m_strLocalSdpType = (eRoType == tmedia_ro_type_offer) ? "answer" : "offer";

    // Start ICE
    if (!iceStart()) {
        return false;
    }

    return true;
}

bool SCSessionCall::iceCreateCtx()
{
    SCAutoLock<SCSessionCall> autoLock(this);

    static tsk_bool_t __use_ice_jingle = tsk_false;
    static tsk_bool_t __use_ipv6 = tsk_false;
    static tsk_bool_t __use_ice_rtcp = tsk_true;

	const char* stun_server_ip = "stun.l.google.com";
	uint16_t stun_server_port = 19302;
	const char* stun_usr_name = tsk_null;
	const char* stun_usr_pwd = tsk_null;
	SC_ASSERT(tmedia_defaults_get_stun_server(&stun_server_ip, &stun_server_port) == 0);
	SC_ASSERT(tmedia_defaults_get_stun_cred(&stun_usr_name, &stun_usr_pwd) == 0);

    if (!m_pIceCtxAudio && (m_eMediaType & SCMediaType_Audio)) {
        m_pIceCtxAudio = tnet_ice_ctx_create(__use_ice_jingle, __use_ipv6, __use_ice_rtcp, tsk_false/*audio*/, &SCSessionCall::iceCallback, this);
        if (!m_pIceCtxAudio) {
            SC_DEBUG_ERROR("Failed to create ICE audio context");
            return false;
        }
        tnet_ice_ctx_set_stun(
            m_pIceCtxAudio,
            stun_server_ip,
            stun_server_port,
            kStunSoftware,
            stun_usr_name,
            stun_usr_pwd
        );
		tnet_ice_ctx_set_stun_enabled(m_pIceCtxAudio, tmedia_defaults_get_icestun_enabled());
		tnet_ice_ctx_set_turn_enabled(m_pIceCtxAudio, tmedia_defaults_get_iceturn_enabled());
    }
    if (!m_pIceCtxVideo && (m_eMediaType & SCMediaType_Video)) {
        m_pIceCtxVideo = tnet_ice_ctx_create(__use_ice_jingle, __use_ipv6, __use_ice_rtcp, tsk_true/*video*/, &SCSessionCall::iceCallback, this);
        if (!m_pIceCtxVideo) {
            SC_DEBUG_ERROR("Failed to create ICE video context");
            return false;
        }
        tnet_ice_ctx_set_stun(
            m_pIceCtxVideo,
            stun_server_ip,
            stun_server_port,
            kStunSoftware,
            stun_usr_name,
            stun_usr_pwd
        );
		tnet_ice_ctx_set_stun_enabled(m_pIceCtxVideo, tmedia_defaults_get_icestun_enabled());
		tnet_ice_ctx_set_turn_enabled(m_pIceCtxVideo, tmedia_defaults_get_iceturn_enabled());
    }

    // For now disable timers until both parties get candidates
    // (RECV ACK) or RECV (200 OK)
    iceSetTimeout(-1);

    return true;
}

bool SCSessionCall::iceSetTimeout(int32_t timeout)
{
    SCAutoLock<SCSessionCall> autoLock(this);

    if (m_pIceCtxAudio) {
        tnet_ice_ctx_set_concheck_timeout(m_pIceCtxAudio, timeout);
    }
    if (m_pIceCtxVideo) {
        tnet_ice_ctx_set_concheck_timeout(m_pIceCtxVideo, timeout);
    }
    return true;
}

bool SCSessionCall::iceGotLocalCandidates(struct tnet_ice_ctx_s *p_IceCtx)
{
    SCAutoLock<SCSessionCall> autoLock(this);

    return (!tnet_ice_ctx_is_active(p_IceCtx) || tnet_ice_ctx_got_local_candidates(p_IceCtx));
}

bool SCSessionCall::iceGotLocalCandidates()
{
    SCAutoLock<SCSessionCall> autoLock(this);

    return iceGotLocalCandidates(m_pIceCtxAudio) && iceGotLocalCandidates(m_pIceCtxVideo);
}

bool SCSessionCall::iceProcessRo(const struct tsdp_message_s* pc_SdpRo, bool isOffer)
{
    SCAutoLock<SCSessionCall> autoLock(this);

    if (!pc_SdpRo) {
        SC_DEBUG_ERROR("Invalid argument");
        return false;
    }

    char* ice_remote_candidates;
    const tsdp_header_M_t* M;
    tsk_size_t index;
    const tsdp_header_A_t *A;
    const char* sess_ufrag = tsk_null;
    const char* sess_pwd = tsk_null;
    int ret = 0, i;

    if (!pc_SdpRo) {
        SC_DEBUG_ERROR("Invalid argument");
        return false;
    }
    if (!m_pIceCtxAudio && !m_pIceCtxVideo) {
        SC_DEBUG_ERROR("Not ready yet");
        return false;
    }

    // session level attributes

    if ((A = tsdp_message_get_headerA(pc_SdpRo, "ice-ufrag"))) {
        sess_ufrag = A->value;
    }
    if ((A = tsdp_message_get_headerA(pc_SdpRo, "ice-pwd"))) {
        sess_pwd = A->value;
    }

    for (i = 0; i < 2; ++i) {
        if ((M = tsdp_message_find_media(pc_SdpRo, i==0 ? "audio": "video"))) { // FIXME: SCreenCast
            const char *ufrag = sess_ufrag, *pwd = sess_pwd;
            ice_remote_candidates = tsk_null;
            index = 0;
            if ((A = tsdp_header_M_findA(M, "ice-ufrag"))) {
                ufrag = A->value;
            }
            if ((A = tsdp_header_M_findA(M, "ice-pwd"))) {
                pwd = A->value;
            }

            while ((A = tsdp_header_M_findA_at(M, "candidate", index++))) {
                tsk_strcat_2(&ice_remote_candidates, "%s\r\n", A->value);
            }
            // ICE processing will be automatically stopped if the remote candidates are not valid
            // ICE-CONTROLLING role if we are the offerer
            ret = tnet_ice_ctx_set_remote_candidates(i==0 ? m_pIceCtxAudio : m_pIceCtxVideo, ice_remote_candidates, ufrag, pwd, !isOffer, tsk_false/*Jingle?*/);
            TSK_SAFE_FREE(ice_remote_candidates);
        }
    }

    return (ret == 0);
}

bool SCSessionCall::iceIsDone()
{
    SCAutoLock<SCSessionCall> autoLock(this);

    return (!tnet_ice_ctx_is_active(m_pIceCtxAudio) || tnet_ice_ctx_is_connected(m_pIceCtxAudio))
           && (!tnet_ice_ctx_is_active(m_pIceCtxVideo) || tnet_ice_ctx_is_connected(m_pIceCtxVideo));
}

bool SCSessionCall::iceIsEnabled(const struct tsdp_message_s* pc_Sdp)
{
    SCAutoLock<SCSessionCall> autoLock(this);

    bool isEnabled = false;
    if (pc_Sdp) {
        int i = 0;
        const tsdp_header_M_t* M;
        while ((M = (tsdp_header_M_t*)tsdp_message_get_headerAt(pc_Sdp, tsdp_htype_M, i++))) {
            isEnabled = true; // at least one "candidate"
            if (!tsdp_header_M_findA(M, "candidate")) {
                return false;
            }
        }
    }

    return isEnabled;
}

bool SCSessionCall::iceStart()
{
    SCAutoLock<SCSessionCall> autoLock(this);

#if 0
    if (!mIceCtxAudio && !mIceCtxVideo) {
        SC_DEBUG_INFO("ICE is not enabled...");
        mIceState = IceStateCompleted;
        return SignalNoMoreIceCandidateToFollow(); // say to the browser that it should not wait for any ICE candidate callback
    }
#endif

    int iRet;

    if ((m_eMediaType & SCMediaType_Audio)) {
        if (m_pIceCtxAudio && (iRet = tnet_ice_ctx_start(m_pIceCtxAudio)) != 0) {
            SC_DEBUG_WARN("tnet_ice_ctx_start() failed with error code = %d", iRet);
            return false;
        }
    }
    if ((m_eMediaType & SCMediaType_Video)) {
        if (m_pIceCtxVideo && (iRet = tnet_ice_ctx_start(m_pIceCtxVideo)) != 0) {
            SC_DEBUG_WARN("tnet_ice_ctx_start() failed with error code = %d", iRet);
            return false;
        }
    }

    return true;
}

int SCSessionCall::iceCallback(const struct tnet_ice_event_s *e)
{
    int ret = 0;
    SCSessionCall *This;

    This = (SCSessionCall *)e->userdata;

    SCAutoLock<SCSessionCall> autoLock(This);

    SC_DEBUG_INFO("ICE callback: %s", e->phrase);

    switch (e->type) {
    case tnet_ice_event_type_started: {
        // This->mIceState = IceStateGathering;
        break;
    }

    case tnet_ice_event_type_gathering_completed:
    case tnet_ice_event_type_conncheck_succeed:
    case tnet_ice_event_type_conncheck_failed:
    case tnet_ice_event_type_cancelled: {
        if (e->type == tnet_ice_event_type_gathering_completed) {
            if (This->iceGotLocalCandidates()) {
                SC_DEBUG_INFO("!!! ICE gathering done !!!");
                //if (This->m_eActionPending == SCCallAction_Make) {
                    This->sendSdp();
                    //This->m_eActionPending = SCCallAction_None;
                //}
                //else if (This->m_eActionPending == SCCallAction_Accept) {

                //}
            }
        }
        else if(e->type == tnet_ice_event_type_conncheck_succeed) {
            if (This->iceIsDone()) {
                // This->mIceState = IceStateConnected;
                // _Utils::PostMessage(This->GetWindowHandle(), WM_ICE_EVENT_CONNECTED, This, (void**)0);
				tmedia_session_mgr_start(This->m_pSessionMgr);
			}
        }
        else if (e->type == tnet_ice_event_type_conncheck_failed || e->type == tnet_ice_event_type_cancelled) {
            // "tnet_ice_event_type_cancelled" means remote party doesn't support ICE -> Not an error
            //This->mIceState = (e->type == tnet_ice_event_type_conncheck_failed) ? IceStateFailed : IceStateClosed;
            //_Utils::PostMessage(This->GetWindowHandle(), WM_ICE_EVENT_CANCELLED, This, (void**)0);
            //if (e->type == tnet_ice_event_type_cancelled)
            //{
            //	This->SignalNoMoreIceCandidateToFollow();
            //}
        }
        break;
    }

    // fatal errors which discard ICE process
    case tnet_ice_event_type_gathering_host_candidates_failed:
    case tnet_ice_event_type_gathering_reflexive_candidates_failed: {
        // This->mIceState = IceStateFailed;
        break;
    }
    }

    return ret;
}

bool SCSessionCall::sendSdp()
{
    SCAutoLock<SCSessionCall> autoLock(this);

    if (!iceGotLocalCandidates()) {
        SC_DEBUG_ERROR("ICE gathering not done");
        return false;
    }

    Json::Value message;

    // Get local now that ICE gathering is done
    const tsdp_message_t* sdp_lo = tmedia_session_mgr_get_lo(m_pSessionMgr);
    if (!sdp_lo) {
        SC_DEBUG_ERROR("Cannot get local offer");
        return false;
    }

    char* sdpStr = tsdp_message_tostring(sdp_lo);
    if (!*sdpStr) {
        SC_DEBUG_ERROR("Cannot serialize local offer");
        return false;
    }
    std::string strSdp(sdpStr);
    TSK_FREE(sdpStr);

    // Compute when transaction identifier for the offer
    m_strTidOffer = SCUtils::randomString();

	message["type"] = m_strLocalSdpType.empty() ? "offer" : m_strLocalSdpType;
    message["cid"] = m_strCallId;
	message["tid"] = m_strTidOffer.empty() ? SCUtils::randomString() : m_strTidOffer;
    message["from"] = SCEngine::s_strCredUserId;
    message["to"] = m_strDestUserId;
    message["sdp"] = strSdp;

    Json::StyledWriter writer;
    std::string output = writer.write(message);
    if (output.empty()) {
        SC_DEBUG_ERROR("Failed serialize JSON content");
        return false;
    }

    if (!m_oSignaling->sendData(output.c_str(), output.length()))
	{
		return false;
	}
	return true;
}

SCObjWrapper<SCSessionCall*> SCSessionCall::newObj(SCObjWrapper<SCSignaling*> oSignaling)
{
    if (!oSignaling) {
        SC_DEBUG_ERROR("Invalid argument");
        return NULL;
    }

    SCObjWrapper<SCSessionCall*> oCall = new SCSessionCall(oSignaling);

    return oCall;
}

SCObjWrapper<SCSessionCall*> SCSessionCall::newObj(SCObjWrapper<SCSignaling*> oSignaling, SCObjWrapper<SCSignalingCallEvent*>& offer)
{
	if (!oSignaling) {
        SC_DEBUG_ERROR("Invalid argument");
        return NULL;
    }
	if (offer->getType() != "offer") {
		SC_DEBUG_ERROR("Call event with type='%s' cannot be used to create a call session", offer->getType().c_str());
        return NULL;
	}

	SCObjWrapper<SCSessionCall*> oCall = new SCSessionCall(oSignaling, offer->getCallId());
	if (oCall)
	{
		oCall->m_strDestUserId = offer->getFrom();
	}

	return oCall;
}
