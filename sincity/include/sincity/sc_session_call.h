#ifndef SINCITY_SESSION_CALL_H
#define SINCITY_SESSION_CALL_H

#include "sc_config.h"
#include "sincity/sc_session.h"
#include "sincity/sc_signaling.h"
#include "sincity/sc_mutex.h"

#include <string>

typedef int SCRoType; // mapped to "enum tmedia_ro_type_e"

class SCSessionCall;

//
//	SCSessionCallIceCallback
//
class SCSessionCallIceCallback : public SCObj
{
public:
	SCSessionCallIceCallback() {
	}
	virtual bool onStateChanged(SCObjWrapper<SCSessionCall*> oCall) = 0;
};

class SCSessionCall : public SCSession
{
    friend class SCAutoLock<SCSessionCall>;
protected:
    SCSessionCall(SCObjWrapper<SCSignaling*> oSignaling, std::string strCallId = "");
public:
    virtual ~SCSessionCall();
    virtual SC_INLINE const char* getObjectId() {
        return "SCSessionCall";
    }

	virtual bool setIceCallback(SCObjWrapper<SCSessionCallIceCallback*> oIceCallback);

    virtual bool setVideoDisplays(SCMediaType_t eVideoType, SCVideoDisplay displayLocal = NULL, SCVideoDisplay displayRemote = NULL);
    virtual bool call(SCMediaType_t eMediaType, std::string strDestUserId);
	
	bool sessionMgrReset();  //  sets current ICE state to "none" AND returns if the *prior* state was "connected"
	bool SC_INLINE sessionMgrIsReady() { 
		return (m_eIceState == SCIceState_Connected); 
	}
	bool sessionMgrStart();
	bool sessionMgrStop();
	bool sessionMgrPause();
	bool sessionMgrResume();
	virtual bool acceptEvent(SCObjWrapper<SCSignalingCallEvent*>& e);
    static bool rejectEvent(SCObjWrapper<SCSignaling*> oSignaling, SCObjWrapper<SCSignalingCallEvent*>& e);

	virtual bool setMute(bool bMuted, SCMediaType_t eMediaType = SCMediaType_All);
    virtual bool hangup();

	virtual bool setVideoFps(int nFps, SCMediaType_t eMediaType = SCMediaType_Video);
	virtual bool setVideoBandwidthUploadMax(int nMax, SCMediaType_t eMediaType = (SCMediaType_t)(SCMediaType_Video | SCMediaType_ScreenCast));
	virtual bool setVideoBandwidthDownloadMax(int nMax, SCMediaType_t eMediaType = (SCMediaType_t)(SCMediaType_Video | SCMediaType_ScreenCast));
	
    virtual SC_INLINE std::string getCallId() {
        return m_strCallId;    /**< Gets the call identifier */
    }
    virtual SC_INLINE SCMediaType_t getMediaType() {
        return m_eMediaType;    /**< Gets the active (negotiated) media type */
    }
	virtual SC_INLINE enum SCIceState_e getIceState() {
		return m_eIceState;
	}

    static SCObjWrapper<SCSessionCall*> newObj(SCObjWrapper<SCSignaling*> signalingSession);
    static SCObjWrapper<SCSessionCall*> newObj(SCObjWrapper<SCSignaling*> signalingSession, SCObjWrapper<SCSignalingCallEvent*>& offer);

private:
    void lock();
    void unlock();

    bool cleanup();

    bool createSessionMgr();
    bool createLocalOffer(const struct tsdp_message_s* pc_Ro = NULL, SCRoType eRoType = 0);
    bool attachVideoDisplays();

    struct tnet_ice_ctx_s* iceCreateCtx(bool bVideo);
    bool iceCreateCtxAll();
    bool iceSetTimeout(int32_t timeout);
    bool iceGotLocalCandidates(struct tnet_ice_ctx_s *p_IceCtx);
    bool iceGotLocalCandidatesAll();
    bool iceProcessRo(const struct tsdp_message_s* pc_SdpRo, bool isOffer);
    bool iceIsDone();
    bool iceIsEnabled(const struct tsdp_message_s* pc_Sdp);
	bool iceStart();
    static int iceCallback(const struct tnet_ice_event_s *e);

    bool sendSdp();

private:
    SCMediaType_t m_eMediaType;

    struct tnet_ice_ctx_s *m_pIceCtxVideo;
	SCIceRole_t m_eIceCtxVideoRole;
    struct tnet_ice_ctx_s *m_pIceCtxScreenCast;
	SCIceRole_t m_eIceCtxScreenCastRole;
    struct tnet_ice_ctx_s *m_pIceCtxAudio;
	SCIceRole_t m_eIceCtxAudioRole;

    struct tmedia_session_mgr_s* m_pSessionMgr;

	uint32_t m_nRoSessionVersion;

    SCObjWrapper<SCMutex*> m_oMutex;

	SCObjWrapper<SCSessionCallIceCallback*> m_oIceCallback;

    SCVideoDisplay m_VideoDisplayLocal;
    SCVideoDisplay m_VideoDisplayRemote;
    SCVideoDisplay m_ScreenCastDisplayLocal;
    SCVideoDisplay m_ScreenCastDisplayRemote;

    std::string m_strDestUserId;
    std::string m_strCallId;
    std::string m_strTidOffer;
    std::string m_strLocalSdpType;

	enum SCIceState_e m_eIceState;

	int m_nVideoBandwidthUploadMax;
	int m_nVideoBandwidthDownloadMax;
	int m_nVideoFps;
};

#endif /* SINCITY_SESSION_CALL_H */
