#import <Foundation/Foundation.h>
#import "sincity/sc_display_osx.h"

#include "sincity/sc_display_osx.h"
#include "sincity/sc_utils.h"
#include "sincity/sc_common.h"
#include "sincity/sc_debug.h"

#include "tinymedia/tmedia_consumer.h"

#include "tsk_memory.h"
#include "tsk_string.h"
#include "tsk_safeobj.h"

#if TARGET_OS_IPHONE
#import "sincity/sc_glview_ios.h"
#endif

@interface SCDisplayOSX : NSObject {
#if TARGET_OS_MAC
    uint8_t* bufferPtr;
#endif
    size_t bufferSize;
    
    int width;
    int height;
    int fps;
    BOOL flipped;
    
    BOOL started;
    BOOL prepared;
    BOOL paused;
    
    const tmedia_consumer_t* consumer;
    
#if TARGET_OS_IPHONE
    SCGlviewIOS* display;
#elif TARGET_OS_MAC
    CGContextRef bitmapContext;
    NSObject<NgnVideoView>* display;
#endif
}

-(SCDisplayOSX*) initWithConsumer: (const tmedia_consumer_t*)consumer;
-(BOOL) isPrepared;
-(BOOL) isStarted;

#if TARGET_OS_IPHONE
-(void) setDisplay: (SCGlviewIOS*)display;
#elif TARGET_OS_MAC
-(void) setDisplay: (NSObject<NgnVideoView>*)display;
#endif

@end

#define kDefaultVideoWidth		176
#define kDefaultVideoHeight		144
#define kDefaultVideoFrameRate	15

@interface SCDisplayOSX(Private)
-(int) drawFrameOnMainThread;
-(int) prepareWithWidth:(int)width_ height:(int)height_ fps:(int)fps_;
-(int) start;
-(int) consumeFrame:(const void*)framePtr size:(unsigned)frameSize;
-(int) pause;
-(int) stop;
-(int) resizeBufferWithWidth:(int)width_ height:(int)height_;
@end

@implementation SCDisplayOSX

-(SCDisplayOSX*) initWithConsumer: (const tmedia_consumer_t*)consumer_ {
    consumer = consumer_;
    bufferPtr = tsk_null, bufferSize = 0;
    width = kDefaultVideoWidth;
    height = kDefaultVideoHeight;
    fps = kDefaultVideoFrameRate;
#if !TARGET_OS_IPHONE
    bitmapContext = nil;
#endif
    return self;
}

-(BOOL) isPrepared {
    return prepared;
}

-(BOOL) isStarted {
    return started;
}

#if TARGET_OS_IPHONE
-(void) setDisplay: (SCGlviewIOS*)display_ {
    if (display != display_) {
        if(display) {
            dispatch_async(dispatch_get_main_queue(), ^{
                [display stopAnimation];
            });
            [display release];
        }
        display = display_ ? [display_ retain] : nil;
        if (display && started) {
            dispatch_async(dispatch_get_main_queue(), ^{
                [display startAnimation];
            });
        }
    }
}
#elif TARGET_OS_MAC
-(void) setDisplay: (NSObject<NgnVideoView>*)display_ {
    [display release];
    display = display_ ? [display_ retain] : nil;
}
#else
#error "Not implemented"
#endif

-(int) prepareWithWidth:(int)width_ height:(int)height_ fps:(int)fps_ {
    SC_DEBUG_INFO_EX(kSCMobuleNameOSXDisplay, "prepareWithWidth(%i,%i,%i)", width_, height_, fps_);
    if (!consumer) {
        SC_DEBUG_ERROR_EX(kSCMobuleNameOSXDisplay, "Invalid embedded consumer");
        return -1;
    }
    
    // resize buffer
    if ([self resizeBufferWithWidth:width_ height:height_] != 0) {
        SC_DEBUG_ERROR("resizeBitmapContextWithWidth:%i andHeight:%i has failed", width_, height_);
        return -1;
    }
    
    // mWidth and and mHeight already updated by resizeBitmap....
    fps = fps_;
    
#if TARGET_OS_IPHONE
    if (display) {
        [display setFps:fps_]; // OpenGL framerate
    }
#elif TARGET_OS_MAC
#endif
    
    prepared = YES;
    return 0;
}

-(int) start {
    SC_DEBUG_INFO_EX(kSCMobuleNameOSXDisplay, "start()");
    started = true;
    if (display) {
        dispatch_async(dispatch_get_main_queue(), ^{
            [display startAnimation];
        });
    }
    return 0;
}

-(int) consumeFrame:(const void*)framePtr size:(unsigned)frameSize {
    // size comparaison is detected: chroma change or width/height change
    // width/height comparaison is to detect: (width,heigh) swapping which would keep size unchanged
    unsigned _frameWidth = (unsigned)TMEDIA_CONSUMER(consumer)->video.display.width;
    unsigned _frameHeight = (unsigned)TMEDIA_CONSUMER(consumer)->video.display.height;
    if (frameSize != bufferSize || width != _frameWidth || height != _frameHeight) {
        
        SC_DEBUG_INFO_EX(kSCMobuleNameOSXDisplay, "Incoming frame size changed: %u->%u, %u->%u, %u->%u", (unsigned)bufferSize, frameSize, width, _frameWidth, height, _frameHeight);
        // resize buffer
        if ([self resizeBufferWithWidth:_frameWidth height:_frameHeight] != 0) {
            SC_DEBUG_ERROR_EX(kSCMobuleNameOSXDisplay, "resizeBufferWithWidth:%i andHeight:%i has failed", _frameWidth, _frameHeight);
            return -1;
        }
    }
    if (display) {
#if TARGET_OS_IPHONE
        [display setBufferYUV:(const uint8_t*)framePtr width:width height:height];
#elif TARGET_OS_MAC
        if (mBitmapContext) {
            memcpy(bufferPtr, framePtr, frameSize);
            CGImageRef imageRef = CGBitmapContextCreateImage(mBitmapContext);
            [display setCurrentImage:imageRef];
            CGImageRelease(imageRef);
        }
#endif
    }
    return 0;
}

-(int) pause {
    SC_DEBUG_INFO_EX(kSCMobuleNameOSXDisplay, "pause()");
    paused = YES;
    if (display) {
        dispatch_async(dispatch_get_main_queue(), ^{
            [display stopAnimation];
        });
    }
    return 0;
}

-(int) stop{
    SC_DEBUG_INFO_EX(kSCMobuleNameOSXDisplay, "stop()");
    started = NO;
    if (display) {
        dispatch_async(dispatch_get_main_queue(), ^{
            [display stopAnimation];
        });
    }
    return 0;
}

-(int) resizeBufferWithWidth:(int)width_ height:(int)height_ {
    if (!consumer) {
        SC_DEBUG_ERROR_EX(kSCMobuleNameOSXDisplay, "Invalid embedded consumer");
        return -1;
    }
    int ret = 0;
    // realloc the buffer
#if TARGET_OS_IPHONE
    unsigned newBufferSize = (width_ * height_ * 3) >> 1; // NV12
#elif TARGET_OS_MAC
    unsigned newBufferSize = (width_ * height_) << 2; // RGB32
    if (!(bufferPtr = (uint8_t*)tsk_realloc(bufferPtr, newBufferSize))) {
        SC_DEBUG_ERROR_EX(kSCMobuleNameOSXDisplay, "Failed to realloc buffer with size=%u", newBufferSize);
        bufferSize = 0;
        return -1;
    }
#endif
    bufferSize = newBufferSize;
    width = width_;
    height = height_;
#if TARGET_OS_IPHONE
#elif TARGET_OS_MAC
    // release context
    CGContextRelease(mBitmapContext), mBitmapContext = nil;
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    mBitmapContext = CGBitmapContextCreate(_mBufferPtr, width_, height_, 8, width_ * 4, colorSpace,
                                           kCGBitmapByteOrder32Little | kCGImageAlphaPremultipliedFirst);
    CGColorSpaceRelease(colorSpace);
#endif
    
    return ret;
}

-(void)dealloc {
    consumer = tsk_null; // you're not the owner
#if TARGET_OS_IPHONE
#elif TARGET_OS_MAC
    TSK_FREE(bufferPtr);
    CGContextRelease(mBitmapContext), bitmapContext = nil;
#endif
    bufferSize = 0;
    
    if (display) {
        [display release];
        display = nil;
    }
    
    [super dealloc];
}

@end


typedef struct sc_display_osx_s {
    TMEDIA_DECLARE_CONSUMER;
    
    SCDisplayOSX* display;
    
    TSK_DECLARE_SAFEOBJ;
}
sc_display_osx_t;

/* ============ Media Consumer Interface ================= */
static int sc_display_osx_set(tmedia_consumer_t *self, const tmedia_param_t* param)
{
    int ret = 0;
    sc_display_osx_t* p_display = (sc_display_osx_t*)self;
    (void)(p_display);
    
    if (!self || !param) {
        SC_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    
    if (param->value_type == tmedia_pvt_int64) {
        if (tsk_striequals(param->key, "remote-hwnd")) {
            SC_DEBUG_INFO_EX(kSCMobuleNameOSXDisplay, "Set remote handle where to display video");
#if TARGET_OS_IPHONE
            SCGlviewIOS* view = reinterpret_cast<SCGlviewIOS*>(*((int64_t*)param->value));
            if (view) {
                SC_ASSERT([view isKindOfClass:[SCGlviewIOS class]]);
            }
#elif TARGET_OS_MAC
            NgnVideoView* view = reinterpret_cast<NgnVideoView*>(*((int64_t*)param->value));
            if (view) {
                SC_ASSERT([view isKindOfClass:[NgnVideoView class]]);
            }
#endif
            tsk_safeobj_lock(p_display);
            [p_display->display setDisplay:view];
            tsk_safeobj_unlock(p_display);
        }
    }
    else if (param->value_type == tmedia_pvt_int32) {
        if (tsk_striequals(param->key, "fullscreen")) {
            SC_DEBUG_INFO_EX(kSCMobuleNameOSXDisplay, "Enable/disable fullscreen");
        }
    }
    
    return ret;
}


static int sc_display_osx_prepare(tmedia_consumer_t* self, const tmedia_codec_t* codec)
{
    sc_display_osx_t* p_display = (sc_display_osx_t*)self;
    int ret = 0;
    
    if (!p_display || !codec || !codec->plugin) {
        SC_DEBUG_ERROR_EX(kSCMobuleNameOSXDisplay, "Invalid parameter");
        return -1;
    }
    
    tsk_safeobj_lock(p_display);
    
    TMEDIA_CONSUMER(p_display)->video.fps = TMEDIA_CODEC_VIDEO(codec)->in.fps;
    TMEDIA_CONSUMER(p_display)->video.in.width = TMEDIA_CODEC_VIDEO(codec)->in.width;
    TMEDIA_CONSUMER(p_display)->video.in.height = TMEDIA_CODEC_VIDEO(codec)->in.height;
    
    SC_DEBUG_INFO_EX(kSCMobuleNameOSXDisplay, "prepare(w=%zu, h=%zu, fps=%u)", TMEDIA_CONSUMER(p_display)->video.in.width,TMEDIA_CONSUMER(p_display)->video.in.height, TMEDIA_CONSUMER(p_display)->video.fps);
    
    ret = [p_display->display prepareWithWidth:(int)TMEDIA_CONSUMER(p_display)->video.in.width height:(int)TMEDIA_CONSUMER(p_display)->video.in.height fps:(int)TMEDIA_CONSUMER(p_display)->video.fps];
    if (ret != 0) {
        goto bail;
    }
    
bail:
    tsk_safeobj_unlock(p_display);
    
    return ret;
}

static int sc_display_osx_start(tmedia_consumer_t* self)
{
    sc_display_osx_t* p_display = (sc_display_osx_t*)self;
    int ret = 0;
    
    SC_DEBUG_INFO_EX(kSCMobuleNameOSXDisplay, "start");
    
    if (!p_display) {
        SC_DEBUG_ERROR_EX(kSCMobuleNameOSXDisplay, "Invalid parameter");
        return -1;
    }
    
    tsk_safeobj_lock(p_display);
    
    if (![p_display->display isPrepared]) {
        SC_DEBUG_ERROR_EX(kSCMobuleNameOSXDisplay, "Not prepared");
        ret = -2;
        goto bail;
    }
    
    if ([p_display->display isStarted]) {
        SC_DEBUG_WARN_EX(kSCMobuleNameOSXDisplay, "Already started");
        goto bail;
    }
    
    ret = [p_display->display start];
    
bail:
    tsk_safeobj_unlock(p_display);
    return ret;
}

static int sc_display_osx_consume(tmedia_consumer_t* self, const void* buffer, tsk_size_t size, const tsk_object_t* proto_hdr)
{
    sc_display_osx_t* p_display = (sc_display_osx_t*)self;
    if (p_display && buffer && size) {
        tsk_safeobj_lock(p_display);
        int ret = [p_display->display consumeFrame:buffer size:(unsigned)size];
        tsk_safeobj_unlock(p_display);
        return ret;
    }
    return -1;
}

static int sc_display_osx_pause(tmedia_consumer_t* self)
{
    sc_display_osx_t* p_display = (sc_display_osx_t*)self;
    int ret = 0;
    
    SC_DEBUG_INFO_EX(kSCMobuleNameOSXDisplay, "pause");
    
    if (!p_display) {
        SC_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    
    tsk_safeobj_lock(p_display);
    
    ret = [p_display->display pause];
    
bail:
    tsk_safeobj_unlock(p_display);
    return ret;
}

static int sc_display_osx_stop(tmedia_consumer_t* self)
{
    sc_display_osx_t* p_display = (sc_display_osx_t*)self;
    int ret = 0;
    
    SC_DEBUG_INFO_EX(kSCMobuleNameOSXDisplay, "stop");
    
    if (!p_display) {
        SC_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    
    tsk_safeobj_lock(p_display);
    
    if (![p_display->display isStarted]) {
        goto bail;
    }
    
    ret = [p_display->display stop];
    
bail:
    tsk_safeobj_unlock(p_display);
    return ret;
}

/* constructor */
static tsk_object_t* sc_display_osx_ctor(tsk_object_t * self, va_list * app)
{
    sc_display_osx_t *p_display = (sc_display_osx_t *)self;
    if (p_display) {
        /* init base */
        tmedia_consumer_init(TMEDIA_CONSUMER(p_display));
#if TARGET_OS_IPHONE /* opengl-es */
        TMEDIA_CONSUMER(p_display)->video.display.chroma = tmedia_chroma_yuv420p;
#else
        TMEDIA_CONSUMER(p_display)->video.display.chroma = tmedia_chroma_rgb32;
#endif
        // Display doesn't need resizing and we want to always match the viewport
        TMEDIA_CONSUMER(p_display)->video.display.auto_resize = tsk_true;
        
        /* init self */
        p_display->display = [[SCDisplayOSX alloc] initWithConsumer:TMEDIA_CONSUMER(p_display)];
        if (!p_display->display) {
            return tsk_null;
        }
        tsk_safeobj_init(p_display);
    }
    return self;
}
/* destructor */
static tsk_object_t* sc_display_osx_dtor(tsk_object_t * self)
{
    sc_display_osx_t *p_display = (sc_display_osx_t *)self;
    if (p_display) {
        /* stop */
        sc_display_osx_stop(TMEDIA_CONSUMER(p_display));
        
        /* deinit base */
        tmedia_consumer_deinit(TMEDIA_CONSUMER(p_display));
        /* deinit self */
        if (p_display->display) {
            [p_display->display release];
            p_display->display = nil;
        }
        tsk_safeobj_deinit(p_display);
        
        SC_DEBUG_INFO_EX(kSCMobuleNameOSXDisplay, "*** destroyed ***");
    }
    
    return self;
}
/* object definition */
static const tsk_object_def_t sc_display_osx_def_s = {
    sizeof(sc_display_osx_t),
    sc_display_osx_ctor,
    sc_display_osx_dtor,
    tsk_null,
};
/* plugin definition*/
static const tmedia_consumer_plugin_def_t sc_display_osx_plugin_def_s = {
    &sc_display_osx_def_s,
    
    tmedia_video,
    "OSX/iOS video display",
    
    sc_display_osx_set,
    sc_display_osx_prepare,
    sc_display_osx_start,
    sc_display_osx_consume,
    sc_display_osx_pause,
    sc_display_osx_stop
};
const tmedia_consumer_plugin_def_t *sc_display_osx_plugin_def_t = &sc_display_osx_plugin_def_s;

