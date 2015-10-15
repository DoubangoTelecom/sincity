#ifndef SINCITY_URL_H
#define SINCITY_URL_H

#include "sc_config.h"
#include "sincity/sc_common.h"
#include "sincity/sc_obj.h"

#include <string>

class SCUrl : public SCObj
{
protected:
    SCUrl(SCUrlType_t eType, std::string strScheme, std::string strHost, std::string strHPath, std::string strSearch, unsigned short nPort, SCUrlHostType_t eHostType);
public:
    virtual ~SCUrl();
    virtual SC_INLINE const char* getObjectId() {
        return "SCUrl";
    }

    virtual SC_INLINE SCUrlType_t getType()const {
        return m_eType;
    }
    virtual SC_INLINE SCUrlHostType_t getHostType()const {
        return m_eHostType;
    }
    virtual SC_INLINE std::string getScheme()const {
        return m_strScheme;
    }
    virtual SC_INLINE std::string getHost()const {
        return m_strHost;
    }
    virtual SC_INLINE std::string getHPath()const {
        return m_strHPath;
    }
    virtual SC_INLINE std::string getSearch()const {
        return m_strSearch;
    }
    virtual SC_INLINE unsigned short getPort()const {
        return m_nPort;
    }

    static SCObjWrapper<SCUrl*> newObj(SCUrlType_t eType, const char* pcScheme, const char* pcHost, const char* pcHPath, const char* pcSearch, unsigned short port, SCUrlHostType_t eHostType);

private:
    SCUrlType_t m_eType;
    SCUrlHostType_t m_eHostType;
    std::string m_strScheme;
    std::string m_strHost;
    std::string m_strHPath;
    std::string m_strSearch;
    unsigned short m_nPort;
};

#endif /* SINCITY_URL_H */
