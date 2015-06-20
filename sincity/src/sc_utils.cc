#include "sincity/sc_utils.h"

#if SC_UNDER_WINDOWS
#	include "tinydav/tdav_win32.h"
#elif SC_UNDER_APPLE
#	include "tinydav/tdav_apple.h"
#endif

#include "tsk_string.h"
#include "tsk_memory.h"
#include "tsk_uuid.h"
#include "tsk_plugin.h"

#include "tnet_utils.h" /* tnet_get_mac_address */

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
		tnet_mac_address address_bytes = { 0 };
		char* address_str = tsk_null;
		int ret = tnet_get_mac_address(&address_bytes);
		if (ret == 0) {
			for (tsk_size_t i = 0; i < sizeof(address_bytes) / sizeof(address_bytes[0]); ++i) {
				tsk_strcat_2(&address_str, "%2.2x", address_bytes[i]);
			}
			__mac_address = std::string(address_str);
			TSK_FREE(address_str);
		}
	}
	return __mac_address;
}
