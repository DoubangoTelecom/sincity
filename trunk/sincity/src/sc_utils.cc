#include "sincity/sc_utils.h"

#if SC_UNDER_WINDOWS
#	include "tinydav/tdav_win32.h"
#elif SC_UNDER_APPLE
#	include "tinydav/tdav_apple.h"
#endif

#include "tsk_uuid.h"
#include "tsk_plugin.h"

std::string SCUtils::randomString()
{
    tsk_uuidstring_t result;
    if (tsk_uuidgenerate(&result) != 0) {
        return std::string("ERROR");
    }
    return std::string(result);
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
