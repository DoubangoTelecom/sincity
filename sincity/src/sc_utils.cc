#include "sincity/sc_utils.h"

#include "tsk_uuid.h"

std::string SCUtils::randomString()
{
    tsk_uuidstring_t result;
    if (tsk_uuidgenerate(&result) != 0) {
        return std::string("ERROR");
    }
    return std::string(result);
}
