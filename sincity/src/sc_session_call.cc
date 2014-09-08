#include "sincity/sc_session_call.h"
#include "sincity/sc_engine.h"

#include "tinymedia.h"

#include "sincity/jsoncpp/sc_json.h"

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

	, m_eActionPending(SCCallAction_None)

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

	SCCallAction_t eOldAction = m_eActionPending;
	m_eActionPending = SCCallAction_Make;

	// Create local offer
	if (!createLocalOffer())
	{
		m_eActionPending = eOldAction;
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

	// Create ICE contexts
	if (!iceCreateCtx())
	{
		return false;
	}

	int iRet;
	tnet_ip_t bestsource;
	if ((iRet = tnet_getbestsource(SCEngine::s_strStunServerAddr.empty() ? "stun.l.google.com" : SCEngine::s_strStunServerAddr.c_str(), SCEngine::s_nStunServerPort ? SCEngine::s_nStunServerPort : 19302, tnet_socket_type_udp_ipv4, &bestsource)))
	{
		SC_DEBUG_ERROR("Failed to get best source [%d]", iRet);
		memcpy(bestsource, "0.0.0.0", 7);
	}

	// Create media session manager
	m_pSessionMgr = tmedia_session_mgr_create(_mediaTypeToNative(m_eMediaType),
            bestsource, tsk_false/*IPv6*/, tsk_true/* offerer */);
	if (!m_pSessionMgr)
	{
		SC_DEBUG_ERROR("Failed to create media session manager");
		return false;
	}

	// Set ICE contexts
	if (tmedia_session_mgr_set_ice_ctx(m_pSessionMgr, m_pIceCtxAudio, m_pIceCtxVideo) != 0)
	{
		SC_DEBUG_ERROR("Failed to set ICE contexts");
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

#if 0
	const tsdp_message_t* sdp_lo = tmedia_session_mgr_get_lo(m_pSessionMgr);
	if (!sdp_lo)
	{
		SC_DEBUG_ERROR("Cannot get local offer");
		return false;
	}
	
	 TSK_OBJECT_SAFE_FREE(m_pSdpLocal);
	 m_pSdpLocal = (tsdp_message_t*)tsk_object_ref((tsk_object_t*)sdp_lo);
	 // mReadyState = (mSdpLocal && tmedia_session_mgr_get_ro(mSessionMgr)) ? ReadyStateActive : ReadyStateOpening;
#endif

	 // Start ICE
	 if (!iceStart())
	 {
		 return false;
	 }

	 return true;
	 //return sendMsgCall();
}

bool SCSessionCall::iceCreateCtx()
{
	static tsk_bool_t __use_ice_jingle = tsk_false;
	static tsk_bool_t __use_ipv6 = tsk_false;
	static tsk_bool_t __use_ice_rtcp = tsk_true;

	if (!m_pIceCtxAudio && (m_eMediaType & SCMediaType_Audio))
	{
		m_pIceCtxAudio = tnet_ice_ctx_create(__use_ice_jingle, __use_ipv6, __use_ice_rtcp, tsk_false/*audio*/, &SCSessionCall::iceCallback, this);
		if (!m_pIceCtxAudio)
		{
			SC_DEBUG_ERROR("Failed to create ICE audio context");
			return false;
		}
		tnet_ice_ctx_set_stun(
			m_pIceCtxAudio, 
			SCEngine::s_strStunServerAddr.c_str(), 
			SCEngine::s_nStunServerPort, 
			kStunSoftware, 
			SCEngine::s_strStunUsername.empty() ? tsk_null : SCEngine::s_strStunUsername.c_str(), 
			SCEngine::s_strStunPassword.empty() ? tsk_null : SCEngine::s_strStunPassword.c_str()
			);
	}
	if (!m_pIceCtxVideo && (m_eMediaType & SCMediaType_Video))
	{
		m_pIceCtxVideo = tnet_ice_ctx_create(__use_ice_jingle, __use_ipv6, __use_ice_rtcp, tsk_true/*video*/, &SCSessionCall::iceCallback, this);
		if (!m_pIceCtxVideo)
		{
			SC_DEBUG_ERROR("Failed to create ICE video context");
			return false;
		}
		tnet_ice_ctx_set_stun(
			m_pIceCtxVideo, 
			SCEngine::s_strStunServerAddr.c_str(), 
			SCEngine::s_nStunServerPort, 
			kStunSoftware, 
			SCEngine::s_strStunUsername.empty() ? tsk_null : SCEngine::s_strStunUsername.c_str(), 
			SCEngine::s_strStunPassword.empty() ? tsk_null : SCEngine::s_strStunPassword.c_str()
			);
	}
	
	// For now disable timers until both parties get candidates
	// (RECV ACK) or RECV (200 OK)
	iceSetTimeout(-1);
	
	return true;
}

bool SCSessionCall::iceSetTimeout(int32_t timeout)
{
	if (m_pIceCtxAudio)
	{
		tnet_ice_ctx_set_concheck_timeout(m_pIceCtxAudio, timeout);
	}
	if (m_pIceCtxVideo)
	{
		tnet_ice_ctx_set_concheck_timeout(m_pIceCtxVideo, timeout);
	}
	return true;
}

bool SCSessionCall::iceGotLocalCandidates(struct tnet_ice_ctx_s *p_IceCtx)
{
	return (!tnet_ice_ctx_is_active(p_IceCtx) || tnet_ice_ctx_got_local_candidates(p_IceCtx));
}

bool SCSessionCall::iceGotLocalCandidates()
{
	return iceGotLocalCandidates(m_pIceCtxAudio) && iceGotLocalCandidates(m_pIceCtxVideo);
}

bool SCSessionCall::iceProcessRo(const struct tsdp_message_s* pc_SdpRo, bool isOffer)
{
	if (!pc_SdpRo)
	{
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

	if (!pc_SdpRo)
	{
		SC_DEBUG_ERROR("Invalid argument");
		return false;
	}
	if (!m_pIceCtxAudio && !m_pIceCtxVideo)
	{
		SC_DEBUG_ERROR("Not ready yet");
		return false;
	}

	// session level attributes
	
	if ((A = tsdp_message_get_headerA(pc_SdpRo, "ice-ufrag")))
	{
		sess_ufrag = A->value;
	}
	if ((A = tsdp_message_get_headerA(pc_SdpRo, "ice-pwd")))
	{
		sess_pwd = A->value;
	}
	
	for (i = 0; i < 2; ++i)
	{
		if ((M = tsdp_message_find_media(pc_SdpRo, i==0 ? "audio": "video"))) // FIXME: SCreenCast
		{
			const char *ufrag = sess_ufrag, *pwd = sess_pwd;
			ice_remote_candidates = tsk_null;
			index = 0;
			if ((A = tsdp_header_M_findA(M, "ice-ufrag")))
			{
				ufrag = A->value;
			}
			if ((A = tsdp_header_M_findA(M, "ice-pwd")))
			{
				pwd = A->value;
			}

			while ((A = tsdp_header_M_findA_at(M, "candidate", index++)))
			{
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
	return (!tnet_ice_ctx_is_active(m_pIceCtxAudio) || tnet_ice_ctx_is_connected(m_pIceCtxAudio))
		&& (!tnet_ice_ctx_is_active(m_pIceCtxVideo) || tnet_ice_ctx_is_connected(m_pIceCtxVideo));
}

bool SCSessionCall::iceIsEnabled(const struct tsdp_message_s* pc_Sdp)
{
	bool isEnabled = false;
	if (pc_Sdp)
	{
		int i = 0;
		const tsdp_header_M_t* M;
		while ((M = (tsdp_header_M_t*)tsdp_message_get_headerAt(pc_Sdp, tsdp_htype_M, i++)))
		{
			isEnabled = true; // at least one "candidate"
			if (!tsdp_header_M_findA(M, "candidate"))
			{
				return false;
			}
		}
	}
	
	return isEnabled;
}

bool SCSessionCall::iceStart()
{
#if 0
	if (!mIceCtxAudio && !mIceCtxVideo)
	{
		SC_DEBUG_INFO("ICE is not enabled...");
		mIceState = IceStateCompleted;
		return SignalNoMoreIceCandidateToFollow(); // say to the browser that it should not wait for any ICE candidate callback
	}
#endif

	int iRet;

	if ((m_eMediaType & SCMediaType_Audio))
	{
		if (m_pIceCtxAudio && (iRet = tnet_ice_ctx_start(m_pIceCtxAudio)) != 0)
		{
			SC_DEBUG_WARN("tnet_ice_ctx_start() failed with error code = %d", iRet);
			return false;
		}
	}
	if ((m_eMediaType & SCMediaType_Video))
	{
		if (m_pIceCtxVideo && (iRet = tnet_ice_ctx_start(m_pIceCtxVideo)) != 0)
		{
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

	SC_DEBUG_INFO("ICE callback: %s", e->phrase);

	switch (e->type)
	{
		case tnet_ice_event_type_started:
			{
				// This->mIceState = IceStateGathering;
				break;
			}

		case tnet_ice_event_type_gathering_completed:
		case tnet_ice_event_type_conncheck_succeed:
		case tnet_ice_event_type_conncheck_failed:
		case tnet_ice_event_type_cancelled:
			{
				if (e->type == tnet_ice_event_type_gathering_completed)
				{
					if (This->iceGotLocalCandidates())
					{
						SC_DEBUG_INFO("!!! ICE gathering done !!!");
						if (This->m_eActionPending == SCCallAction_Make)
						{
							This->sendMsgCall();
							This->m_eActionPending = SCCallAction_None;
						}
						else if (This->m_eActionPending == SCCallAction_Accept)
						{
							
						}
					}
				}
				else if(e->type == tnet_ice_event_type_conncheck_succeed)
				{
					if (This->iceIsDone())
					{
						// This->mIceState = IceStateConnected;
						// _Utils::PostMessage(This->GetWindowHandle(), WM_ICE_EVENT_CONNECTED, This, (void**)0);
					}
				}
				else if (e->type == tnet_ice_event_type_conncheck_failed || e->type == tnet_ice_event_type_cancelled)
				{
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
		case tnet_ice_event_type_gathering_reflexive_candidates_failed:
			{
				// This->mIceState = IceStateFailed;
				break;
			}
	}
	
	return ret;
}

bool SCSessionCall::sendMsgCall()
{
	if (!iceGotLocalCandidates())
	{
		SC_DEBUG_ERROR("ICE gathering not done");
		return false;
	}

	Json::Value message;

	// Get local now that ICE gathering is done
	const tsdp_message_t* sdp_lo = tmedia_session_mgr_get_lo(m_pSessionMgr);
	if (!sdp_lo)
	{
		SC_DEBUG_ERROR("Cannot get local offer");
		return false;
	}

	char* sdpStr = tsdp_message_tostring(sdp_lo);
	if (!*sdpStr)
	{
		SC_DEBUG_ERROR("Cannot serialize local offer");
		return false;
	}
	std::string strSdp(sdpStr);
	TSK_FREE(sdpStr);

	message["type"] = "offer";
	message["cid"] = "fake-callid";
	message["tid"] = "fake-transaction-id";
	message["from"] = "001";
	message["to"] = "002";
	message["sdp"] = strSdp;	

	Json::StyledWriter writer;
	std::string output = writer.write(message);
	if (output.empty())
	{
		SC_DEBUG_ERROR("Failed serialize JSON content");
		return false;
	}

	return m_oSignaling->sendData(output.c_str(), output.length());
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
