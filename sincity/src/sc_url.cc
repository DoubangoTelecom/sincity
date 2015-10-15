#include "sincity/sc_url.h"

#include "tsk_string.h"

SCUrl::SCUrl(SCUrlType_t eType, std::string strScheme, std::string strHost, std::string strHPath, std::string strSearch, unsigned short nPort, SCUrlHostType_t eHostType)
    : m_eType(eType)
    , m_strScheme(strScheme)
    , m_strHost(strHost)
    , m_strHPath(strHPath)
    , m_strSearch(strSearch)
    , m_nPort(nPort)
    , m_eHostType(eHostType)
{

}

SCUrl::~SCUrl()
{
    SC_DEBUG_INFO("*** SCUrl destroyed ***");
}

SCObjWrapper<SCUrl*> SCUrl::newObj(SCUrlType_t eType, const char* pcScheme, const char* pcHost, const char* pcHPath, const char* pcSearch, unsigned short port, SCUrlHostType_t eHostType)
{
    if (tsk_strnullORempty(pcScheme)) {
        SC_DEBUG_ERROR("Url scheme is null or empty");
        return NULL;
    }
    if (tsk_strnullORempty(pcHost)) {
        SC_DEBUG_ERROR("Url host is null or empty");
        return NULL;
    }
    if (!port) {
        SC_DEBUG_ERROR("Url port is equal to zero");
        return NULL;
    }

    return new SCUrl(
               eType,
               std::string(pcScheme),
               std::string(pcHost),
               std::string(pcHPath ? pcHPath : ""),
               std::string(pcSearch ? pcSearch : ""),
               port,
               eHostType);
}
