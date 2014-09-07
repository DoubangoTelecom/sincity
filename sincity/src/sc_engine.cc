#include "sincity/sc_engine.h"

#include "tsk_debug.h"

#include "tinynet.h"

#include "tinydav.h"

bool SCEngine::s_bInitialized = false;

SCEngine::SCEngine()
{

}

SCEngine::~SCEngine()
{

}

bool SCEngine::init()
{
    if (!s_bInitialized) {
        if (tnet_startup() != 0) {
            return false;
        }

        if (tdav_init() != 0) {
            return false;
        }
        s_bInitialized = true;
    }

    return true;
}

bool SCEngine::deInit()
{
    if (s_bInitialized) {
        tdav_deinit();
        tnet_cleanup();

        s_bInitialized = false;
    }

    return true;
}

bool SCEngine::setDebugLevel(SCDebugLevel_t eLevel)
{
    tsk_debug_set_level((int)eLevel);
    return true;
}