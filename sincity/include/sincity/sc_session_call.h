#ifndef SINCITY_SESSION_CALL_H
#define SINCITY_SESSION_CALL_H

#include "sc_config.h"
#include "sincity/sc_session.h"
#include "sincity/sc_signaling.h"
#include "sincity/sc_mutex.h"

#include <string>

class SCSessionCall : public SCSession
{
	friend class SCAutoLock<SCSessionCall>;
protected:
	SCSessionCall(SCObjWrapper<SCSignaling*> oSignaling, std::string strCallId = "");
public:
	virtual ~SCSessionCall();
	virtual SC_INLINE const char* getObjectId() { return "SCSessionCall"; }
	
	virtual bool call(SCMediaType_t eMediaType, std::string strDestUserId);
	virtual bool handEvent(SCObjWrapper<SCSignalingCallEvent*>& e);

	virtual SC_INLINE std::string getCallId() { return m_strCallId; }
	
	static SCObjWrapper<SCSessionCall*> newObj(SCObjWrapper<SCSignaling*> oSignaling);
	static SCObjWrapper<SCSessionCall*> newObj(SCObjWrapper<SCSignaling*> oSignaling, SCObjWrapper<SCSignalingCallEvent*>& e);

private:
	void lock();
	void unlock();

	bool hangup();

	bool createSessionMgr();
	bool createLocalOffer(const struct tsdp_message_s* pc_Ro = NULL, enum tmedia_ro_type_e eRoType = (enum tmedia_ro_type_e)0);

	bool iceCreateCtx();
	bool iceSetTimeout(int32_t timeout);
	bool iceGotLocalCandidates(struct tnet_ice_ctx_s *p_IceCtx);
	bool iceGotLocalCandidates();
	bool iceProcessRo(const struct tsdp_message_s* pc_SdpRo, bool isOffer);
	bool iceIsDone();
	bool iceIsEnabled(const struct tsdp_message_s* pc_Sdp);
	bool iceStart();
	static int iceCallback(const struct tnet_ice_event_s *e);

	bool sendSdp();

private:
	SCMediaType_t m_eMediaType;

	struct tnet_ice_ctx_s *m_pIceCtxVideo;
	struct tnet_ice_ctx_s *m_pIceCtxAudio;

	struct tmedia_session_mgr_s* m_pSessionMgr;

	SCObjWrapper<SCMutex*> m_oMutex;

	std::string m_strDestUserId;
	std::string m_strCallId;
	std::string m_strTidOffer;
	std::string m_strLocalSdpType;
};

#endif /* SINCITY_SESSION_CALL_H */
