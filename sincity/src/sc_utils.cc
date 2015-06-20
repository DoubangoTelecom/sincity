#include "sincity/sc_utils.h"

#if SC_UNDER_WINDOWS
#	include <windows.h>
#	include <iphlpapi.h>
#	include "tinydav/tdav_win32.h"
#elif SC_UNDER_APPLE
#	include "tinydav/tdav_apple.h"
#endif

#include "tsk_string.h"
#include "tsk_memory.h"
#include "tsk_uuid.h"
#include "tsk_plugin.h"

std::string SCUtils::randomString()
{
    tsk_uuidstring_t result;
	static std::string __mac_address = "";
	if (__mac_address.empty()) {
		__mac_address = SCUtils::macAddress();
	}

    if (tsk_uuidgenerate(&result) != 0) {
        return std::string("ERROR");
    }
	return __mac_address + "-" + std::string(result);
}

bool SCUtils::fileExists(const char* path)
{
#define _file_exists(path) tsk_plugin_file_exist((path))
    return _file_exists(path);
}

const char* SCUtils::currentDirectoryPath()
{
#if SC_UNDER_WINDOWS
    return tdav_get_current_directory_const();
#else
    return ".";
#endif
}

std::string SCUtils::macAddress()
{
	static std::string __mac_address = "";
	if (__mac_address.empty()) {
#if SC_UNDER_WINDOWS
		IP_ADAPTER_INFO *info = NULL, *pos;
		DWORD size = 0;
		ULONG ret;
		char* address = tsk_null;

		if ((ret = GetAdaptersInfo(info, &size)) == ERROR_SUCCESS || ret == ERROR_BUFFER_OVERFLOW) {
			info = (IP_ADAPTER_INFO *)tsk_calloc(size + 1, 1);
			if (info) {
				if ((ret = GetAdaptersInfo(info, &size)) == ERROR_SUCCESS) {
					for (pos = info; pos != NULL && address == tsk_null; pos = pos->Next) {
						for (UINT i = 0; i < pos->AddressLength; ++i) {
							tsk_strcat_2(&address, "%2.2x", pos->Address[i]);
						}
					}
				}
			}
			__mac_address = std::string(address);
			TSK_FREE(info);
			TSK_FREE(address);
		}
#endif
	}
	return __mac_address;
}
