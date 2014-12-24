#ifndef SINCITY_UTILS_H
#define SINCITY_UTILS_H

#include "sc_config.h"
#include "sincity/sc_common.h"

#include <string>

class SCUtils
{
public:
    static std::string randomString();
    static bool fileExists(const char* path);
    static const char* currentDirectoryPath();
};

#endif /* SINCITY_UTILS_H */
