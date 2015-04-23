#include "sincity/sc_display_fake.h"
#include "sincity/sc_utils.h"
#include "sincity/sc_common.h"
#include "sincity/sc_debug.h"

#include "tinymedia/tmedia_consumer.h"

#include "tsk_string.h"
#include "tsk_safeobj.h"

typedef struct sc_display_fake_s {
    TMEDIA_DECLARE_CONSUMER;

    tsk_bool_t b_started;
    tsk_bool_t b_prepared;

    TSK_DECLARE_SAFEOBJ;
}
sc_display_fake_t;



/* ============ Media Consumer Interface ================= */
static int sc_display_fake_set(tmedia_consumer_t *self, const tmedia_param_t* param)
{
    int ret = 0;
    sc_display_fake_t* p_display = (sc_display_fake_t*)self;
    (p_display);

    if (!self || !param) {
        SC_DEBUG_ERROR("Invalid parameter");
        return -1;
    }

    if (param->value_type == tmedia_pvt_int64) {
        if (tsk_striequals(param->key, "remote-hwnd")) {
            SC_DEBUG_INFO_EX(kSCMobuleNameFakeDisplay, "Set remote handle where to display video");
        }
    }
    else if (param->value_type == tmedia_pvt_int32) {
        if (tsk_striequals(param->key, "fullscreen")) {
            SC_DEBUG_INFO_EX(kSCMobuleNameFakeDisplay, "Enable/disable fullscreen");
        }
    }

    return ret;
}


static int sc_display_fake_prepare(tmedia_consumer_t* self, const tmedia_codec_t* codec)
{
    sc_display_fake_t* p_display = (sc_display_fake_t*)self;

    SC_DEBUG_INFO_EX(kSCMobuleNameFakeDisplay, "prepare");

    if (!p_display || !codec || !codec->plugin) {
        SC_DEBUG_ERROR_EX(kSCMobuleNameFakeDisplay, "Invalid parameter");
        return -1;
    }

    tsk_safeobj_lock(p_display);

    TMEDIA_CONSUMER(p_display)->video.fps = TMEDIA_CODEC_VIDEO(codec)->in.fps;
    TMEDIA_CONSUMER(p_display)->video.in.width = TMEDIA_CODEC_VIDEO(codec)->in.width;
    TMEDIA_CONSUMER(p_display)->video.in.height = TMEDIA_CODEC_VIDEO(codec)->in.height;

// Defines what we want for incoming size (up to Doubango video engine to resize decoded stream to make you happy)
    if (!TMEDIA_CONSUMER(p_display)->video.display.width) {
        TMEDIA_CONSUMER(p_display)->video.display.width = TMEDIA_CONSUMER(p_display)->video.in.width;
    }
    if (!TMEDIA_CONSUMER(p_display)->video.display.height) {
        TMEDIA_CONSUMER(p_display)->video.display.height = TMEDIA_CONSUMER(p_display)->video.in.height;
    }

    p_display->b_prepared = tsk_true;

    tsk_safeobj_unlock(p_display);

    return 0;
}

static int sc_display_fake_start(tmedia_consumer_t* self)
{
    sc_display_fake_t* p_display = (sc_display_fake_t*)self;
    int ret = 0;

    SC_DEBUG_INFO_EX(kSCMobuleNameFakeDisplay, "start");

    if (!p_display) {
        SC_DEBUG_ERROR_EX(kSCMobuleNameFakeDisplay, "Invalid parameter");
        return -1;
    }

    tsk_safeobj_lock(p_display);

    if (!p_display->b_prepared) {
        SC_DEBUG_ERROR_EX(kSCMobuleNameFakeDisplay, "Not prepared");
        ret = -2;
        goto bail;
    }

    if (p_display->b_started) {
        SC_DEBUG_WARN_EX(kSCMobuleNameFakeDisplay, "Already started");
        goto bail;
    }

    p_display->b_started = tsk_true;

bail:
    tsk_safeobj_unlock(p_display);
    return ret;
}

static int sc_display_fake_consume(tmedia_consumer_t* self, const void* buffer, tsk_size_t size, const tsk_object_t* proto_hdr)
{
    sc_display_fake_t* p_display = (sc_display_fake_t*)self;
    SC_DEBUG_INFO_EX(kSCMobuleNameFakeDisplay, "consume");
    if (p_display && buffer && size) {
        tsk_safeobj_lock(p_display);
        SC_DEBUG_INFO_EX(kSCMobuleNameFakeDisplay, "Skipping video buffer: %ux%u", (unsigned)TMEDIA_CONSUMER(p_display)->video.display.width, (unsigned)TMEDIA_CONSUMER(p_display)->video.display.height);
        tsk_safeobj_unlock(p_display);
        return 0;
    }
    return -1;
}

static int sc_display_fake_pause(tmedia_consumer_t* self)
{
    sc_display_fake_t* p_display = (sc_display_fake_t*)self;

    SC_DEBUG_INFO_EX(kSCMobuleNameFakeDisplay, "pause");

    if (!p_display) {
        SC_DEBUG_ERROR("Invalid parameter");
        return -1;
    }

    tsk_safeobj_lock(p_display);

    tsk_safeobj_unlock(p_display);

    return 0;
}

static int sc_display_fake_stop(tmedia_consumer_t* self)
{
    sc_display_fake_t* p_display = (sc_display_fake_t*)self;
    int ret = 0;

    SC_DEBUG_INFO_EX(kSCMobuleNameFakeDisplay, "stop");

    if (!p_display) {
        SC_DEBUG_ERROR("Invalid parameter");
        return -1;
    }

    tsk_safeobj_lock(p_display);

    if (!p_display->b_started) {
        goto bail;
    }

    p_display->b_started = tsk_false;

bail:
    tsk_safeobj_unlock(p_display);
    return ret;
}

/* constructor */
static tsk_object_t* sc_display_fake_ctor(tsk_object_t * self, va_list * app)
{
    sc_display_fake_t *p_display = (sc_display_fake_t *)self;
    if (p_display) {
        /* init base */
        tmedia_consumer_init(TMEDIA_CONSUMER(p_display));
        TMEDIA_CONSUMER(p_display)->video.display.chroma = tmedia_chroma_yuv420p; // We want I420 frames
#if 0 // Display doesn't need resizing?
        TMEDIA_CONSUMER(p_display)->video.display.auto_resize = tsk_true;
#endif

        /* init self */
        tsk_safeobj_init(p_display);
    }
    return self;
}
/* destructor */
static tsk_object_t* sc_display_fake_dtor(tsk_object_t * self)
{
    sc_display_fake_t *p_display = (sc_display_fake_t *)self;
    if (p_display) {
        /* stop */
        if (p_display->b_started) {
            sc_display_fake_stop(TMEDIA_CONSUMER(p_display));
        }
        /* deinit base */
        tmedia_consumer_deinit(TMEDIA_CONSUMER(p_display));
        /* deinit self */
        tsk_safeobj_deinit(p_display);

        SC_DEBUG_INFO_EX(kSCMobuleNameFakeDisplay, "*** destroyed ***");
    }

    return self;
}
/* object definition */
static const tsk_object_def_t sc_display_fake_def_s = {
    sizeof(sc_display_fake_t),
    sc_display_fake_ctor,
    sc_display_fake_dtor,
    tsk_null,
};
/* plugin definition*/
static const tmedia_consumer_plugin_def_t sc_display_fake_plugin_def_s = {
    &sc_display_fake_def_s,

    tmedia_video,
    "Fake video display",

    sc_display_fake_set,
    sc_display_fake_prepare,
    sc_display_fake_start,
    sc_display_fake_consume,
    sc_display_fake_pause,
    sc_display_fake_stop
};
const tmedia_consumer_plugin_def_t *sc_display_fake_plugin_def_t = &sc_display_fake_plugin_def_s;

