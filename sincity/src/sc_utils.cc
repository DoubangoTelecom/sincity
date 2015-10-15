#include "sincity/sc_utils.h"

#if SC_UNDER_WINDOWS
#	include "tinydav/tdav_win32.h"
#elif SC_UNDER_APPLE
#	include "tinydav/tdav_apple.h"
#   import <UIKit/UIKit.h>
#   import <Foundation/Foundation.h>
#endif

#if SC_UNDER_IPHONE || SC_UNDER_IPHONE_SIMULATOR
#endif

#include "tsk_string.h"
#include "tsk_memory.h"
#include "tsk_uuid.h"
#include "tsk_plugin.h"

#include "tnet_utils.h" /* tnet_get_mac_address */

std::string SCUtils::randomString()
{
    tsk_uuidstring_t result;
    if (tsk_uuidgenerate(&result) != 0) {
        return std::string("ERROR");
    }
    return SCUtils::deviceIdentifier() + "-" + std::string(result);
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

std::string SCUtils::deviceIdentifier()
{
    static std::string __device_identifier = "";
#if SC_UNDER_IPHONE || SC_UNDER_IPHONE_SIMULATOR
    /*In iOS 7 and later, if you ask for the MAC address of an iOS device, the system returns the value 02:00:00:00:00:00. If you need to identify the device, use the identifierForVendor property of UIDevice instead. (Apps that need an identifier for their own advertising purposes should consider using the advertisingIdentifier property of ASIdentifierManager instead.)"
     */
    if ([[[UIDevice currentDevice] systemVersion] floatValue] >= 7) {
        __device_identifier = std::string([[UIDevice currentDevice].identifierForVendor.UUIDString UTF8String]);
    }
#endif /* SC_UNDER_IPHONE || SC_UNDER_IPHONE_SIMULATOR */
    if (__device_identifier.empty()) {
        tnet_mac_address address_bytes = { 0 };
        char* address_str = tsk_null;
        int ret = tnet_get_mac_address(&address_bytes);
        if (ret == 0) {
            for (tsk_size_t i = 0; i < sizeof(address_bytes) / sizeof(address_bytes[0]); ++i) {
                tsk_strcat_2(&address_str, "%2.2x", address_bytes[i]);
            }
            __device_identifier = std::string(address_str);
            TSK_FREE(address_str);
        }
    }
    return __device_identifier;
}
