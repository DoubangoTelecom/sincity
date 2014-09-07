#ifndef SINCITY_SESSION_CALL_H
#define SINCITY_SESSION_CALL_H

#include "sc_config.h"
#include "sincity/sc_session.h"
#include "sincity/sc_signaling.h"

#include <string>

class SCSessionCall : public SCSession
{
protected:
	SCSessionCall(std::string strUserId, SCObjWrapper<SCSignaling*> oSignaling);
public:
	virtual ~SCSessionCall();
	virtual SC_INLINE const char* getObjectId() { return "SCSessionCall"; }
	
	virtual bool call(SCMediaType_t eMediaType, std::string srtDestUserId);
	virtual bool hangup();
	
	static SCObjWrapper<SCSessionCall*> newObj(std::string strUserId, SCObjWrapper<SCSignaling*> oSignaling);

private:
	bool createSessionMgr();
	bool createLocalOffer();

private:
	SCMediaType_t m_eMediaType;
	struct tnet_ice_ctx_s *m_pIceCtxVideo;
	struct tnet_ice_ctx_s *m_pIceCtxAudio;

	struct tmedia_session_mgr_s* m_pSessionMgr;

	struct tsdp_message_s* m_pSdpLocal;
	struct tsdp_message_s* m_pSdpRemote;
};


#endif /* SINCITY_SESSION_CALL_H */
