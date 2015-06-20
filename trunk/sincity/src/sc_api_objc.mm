#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import "sincity/sc_api_objc.h"
#import "sincity/sc_api.h"

#define kTAG "Objc Wrapper"

@class Signaling;

@interface SignalingCallEvent: NSObject<SCObjcSignalingCallEvent> {
@public SCObjWrapper<SCSignalingCallEvent* >event;
}
+(id<SCObjcSignalingCallEvent>) createWithEvent:(SCObjWrapper<SCSignalingCallEvent* >)event;
@end

@interface Session: NSObject<SCObjcSession> {
@protected SCObjcSessionType type;
@protected Signaling* signaling;
}
+(SCObjcMediaType) mediaTypeFromNative:(SCMediaType_t)type;
+(SCMediaType_t) mediaTypeToNative:(SCObjcMediaType)type;
-(Session*) initWithType:(SCObjcSessionType)type signaling:(id<SCObjcSignaling>)signaling;
@end

@interface SessionCall: Session<SCObjcSessionCall> {
    SCObjWrapper<SCSessionCall*> call;
    NSObject<SCObjcSessionCallDelegate>* delegate;
}
-(SessionCall*)initWithSignaling:(id<SCObjcSignaling>)signaling  offer:(id<SCObjcSignalingCallEvent>)e;
-(SessionCall*)initWithSignaling:(id<SCObjcSignaling>)signaling;
-(void) onAudioSessionInterruptionEvent:(NSNotification*)notif;
#if __IPHONE_OS_VERSION_MIN_REQUIRED < 6000
-(void)beginInterruption;
-(void)endInterruption;
#endif /* __IPHONE_OS_VERSION_MIN_REQUIRED < 6000 */
@end

@interface Signaling : NSObject<SCObjcSignaling> {
@public SCObjWrapper<SCSignaling* >signalSession;
}
-(Signaling*) initWithConnectionUrl:(const NSString*)connectionUrl localIP:(const NSString *)strLocalIP localPort:(const unsigned short)uLocalPort;
@end

@interface IceServer : NSObject<SCObjcIceServer> {
@public
    NSString* protocol;
    NSString* host;
    unsigned short port;
    BOOL turnEnabled;
    BOOL stunEnabled;
    NSString* login;
    NSString* password;
}
@end

@interface Config : NSObject<SCObjcConfig> {
    Json::Value jsonConfig;
}
-(Config*)initWithFile:(const NSString*)fullPath;
@end

@interface Viewport: NSObject<SCObjcViewport> {
    int x;
    int y;
    int width;
    int height;
}
-(Viewport*)initWithCoor:(int)iX y:(int)iY width:(int)iWidth height:(int)iHeight;
@end

//
//  StringUtils
//
static NSString* toNSString(const char* cstring) {
    return cstring ? [NSString stringWithCString:cstring encoding: NSUTF8StringEncoding] : nil;
}
static NSString* toNSString(const std::string& stdstring) {
    return toNSString(stdstring.c_str());
}
static const char* toCString(const NSString* nsstring) {
    return nsstring ? [nsstring UTF8String] : NULL;
}
static std::string toStdString(const NSString* nsstring) {
    const char* csting =  toCString(nsstring);
    return std::string(csting ? csting : "");
}

//
//  SCSessionCallIceCallbackDummy
//
class SCSessionCallIceCallbackDummy : public SCSessionCallIceCallback
{
protected:
    SCSessionCallIceCallbackDummy(id<SCObjcSessionCallDelegate> delegate_) {
        delegate = [delegate_ retain];
    }
public:
    virtual ~SCSessionCallIceCallbackDummy() {
        SC_DEBUG_INFO_EX(kTAG, "*** SCSessionCallIceCallbackDummy destroyed ***");
        [delegate release];
        delegate = nil;
    }
    virtual SC_INLINE const char* getObjectId() {
        return "SCSessionCallIceCallbackDummy";
    }
    virtual bool onStateChanged(SCObjWrapper<SCSessionCall*> oCall) {
        if (delegate && [delegate respondsToSelector:@selector(callIceStateChanged)]) {
            return [delegate callIceStateChanged];
        }
        return true;
    }
    static SCObjWrapper<SCSessionCallIceCallback*> newObj(id<SCObjcSessionCallDelegate> delegate_) {
        return new SCSessionCallIceCallbackDummy(delegate_);
    }
private:
    NSObject<SCObjcSessionCallDelegate>* delegate;
};

//
// SCSignalingCallbackDummy
//
class SCSignalingCallbackDummy : public SCSignalingCallback
{
protected:
    SCSignalingCallbackDummy(id<SCObjcSignalingDelegate> delegate_) {
        delegate = [delegate_ retain];
    }
public:
    virtual ~SCSignalingCallbackDummy() {
        SC_DEBUG_INFO_EX(kTAG, "*** ~SCSignalingCallbackDummy destroyed ***");
        [delegate release];
        delegate = nil;
    }
    virtual bool onEventNet(SCObjWrapper<SCSignalingEvent*>& e_) {
        if (delegate) {
            switch (e_->getType()) {
                case SCSignalingEventType_NetConnected: {
                    SC_DEBUG_INFO_EX(kTAG, "***Signaling module connected ***");
                    break;
                }
                case SCSignalingEventType_NetReady: {
                    SC_DEBUG_INFO_EX(kTAG, "***Signaling module ready ***");
                    if ([delegate respondsToSelector:@selector(signalingDidConnect:)]) {
                        [delegate signalingDidConnect:toNSString(e_->getDescription())];
                    }
                    break;
                }
                case SCSignalingEventType_NetDisconnected:
                case SCSignalingEventType_NetError: {
                    SC_DEBUG_INFO_EX(kTAG, "***Signaling module disconnected ***");
                    if ([delegate respondsToSelector:@selector(signalingDidDisconnect:)]) {
                        [delegate signalingDidDisconnect:toNSString(e_->getDescription())];
                    }
                    break;
                }
                case SCSignalingEventType_NetData: {
                    SC_DEBUG_INFO_EX(kTAG, "***Signaling module [%s] DATA:%.*s ***", e_->getDescription().c_str(), (int)e_->getDataSize(), (const char*)e_->getDataPtr());
                    if (e_->getDescription() == "chat") {
                        if ([delegate respondsToSelector:@selector(signalingGotChatMessage: username:)]) {
                            Json::Reader reader;
                            Json::Value chat;
                            if (reader.parse(std::string((const char*)e_->getDataPtr(), e_->getDataSize()), chat, false)) {
                                if (chat["message"].isString() && chat["username"].isString()) {
                                    [delegate signalingGotChatMessage:toNSString(chat["message"].asCString()) username:toNSString(chat["username"].asCString())];
                                }
                            }
                        }
                    }
                    if ([delegate respondsToSelector:@selector(signalingGotData:)]) {
                        [delegate signalingGotData:[[[NSData alloc] initWithBytes:e_->getDataPtr() length:e_->getDataSize()] autorelease]];
                    }
                    break;
                }
                default: break;
            }
        }
        
        return true;
    }
    virtual SC_INLINE const char* getObjectId() {
        return "SCSignalingCallbackDummy";
    }
    virtual bool onEventCall(SCObjWrapper<SCSignalingCallEvent*>& e) {
        //!\Deadlock issue: You must not call any function from 'SCSignaling' class unless you fork a new thread.
        if (delegate && [delegate respondsToSelector:@selector(signalingGotEventCall:)]) {
            return [delegate signalingGotEventCall:[SignalingCallEvent createWithEvent:e]];
        }
        return true;
    }
    
    static SCObjWrapper<SCSignalingCallback*> newObj(id<SCObjcSignalingDelegate> delegate_) {
        return new SCSignalingCallbackDummy(delegate_);
    }
private:
    NSObject<SCObjcSignalingDelegate>* delegate;
};


//
//  IceServer
//
@implementation IceServer
-(NSString*) protocol {
    return protocol;
}
-(NSString*) host {
    return host;
}
-(unsigned short) port {
    return port;
}
-(BOOL) turnEnabled {
    return turnEnabled;
}
-(BOOL) stunEnabled {
    return stunEnabled;
}
-(NSString*) login {
    return login;
}
-(NSString*) password {
    return password;
}
@end

///
/// Viewport
///
@implementation Viewport
-(Viewport*)initWithCoor:(int)iX y:(int)iY width:(int)iWidth height:(int)iHeight {
    [super init];
    x = iX, y = iY, height = iHeight, width = iWidth;
    return self;
}
-(int) x { return x; }
-(int) y { return y; };
-(int) width { return width; }
-(int) height { return height; }
@end


//
//  Config
//
@implementation Config
-(Config*)initWithFile:(const NSString*)fullPath {
    [super init];
    SC_ASSERT(fullPath != nil);
    
    FILE* p_file = fopen(toCString(fullPath), "rb");
    SC_ASSERT(p_file);
    
    fseek(p_file, 0, SEEK_END);
    long fsize = ftell(p_file);
    fseek(p_file, 0, SEEK_SET);
    
    char *p_buffer = new char[fsize + 1];
    SC_ASSERT(p_buffer);
    p_buffer[fsize] = 0;
    SC_ASSERT(fread(p_buffer, 1, fsize, p_file) == fsize);
    std::string jsonText(p_buffer);
    fclose(p_file);
    delete[] p_buffer;
    
    Json::Reader reader;
    SC_ASSERT(reader.parse(jsonText, jsonConfig, false));
    
    SC_DEBUG_INFO_EX(kTAG, "loading config file = %s", jsonConfig.toStyledString().c_str());
    
    return self;
}

-(BOOL) debugLevel:(SCObjcDebugLevel*)level {
    *level = jsonConfig["debug_level"].isNumeric() ? (SCObjcDebugLevel)jsonConfig["debug_level"].asInt() : SCObjcDebugLevelInfo;
    return YES;
}

-(BOOL) localID:(NSString**)strId {
    *strId = jsonConfig["local_id"].isString() ? toNSString(jsonConfig["local_id"].asCString()) : @"001";
    return YES;
}

-(BOOL) remoteID:(NSString**)strId {
    *strId = jsonConfig["remote_id"].isString() ? toNSString(jsonConfig["remote_id"].asCString()) : @"002";
    return YES;
}

-(BOOL) connectionUrl:(NSString**)strUrl {
    if (jsonConfig["connection_url"].isString()) {
        *strUrl = toNSString(jsonConfig["connection_url"].asCString());
        return YES;
    }
    return NO;
}

-(BOOL) videoPrefSize:(NSString**)strPrefSize {
    if (jsonConfig["video_pref_size"].isString()) {
        *strPrefSize = toNSString(jsonConfig["video_pref_size"].asCString());
        return YES;
    }
    return NO;
}

-(BOOL) videoFps:(int*)fps {
    if (jsonConfig["video_fps"].isNumeric()) {
        *fps = jsonConfig["video_fps"].asInt();
        return YES;
    }
    return NO;
}

-(BOOL) videoBandwidthUpMax:(int*)bandwidth {
    if (jsonConfig["video_bandwidth_up_max"].isNumeric()) {
        *bandwidth = jsonConfig["video_bandwidth_up_max"].asInt();
        return YES;
    }
    return NO;
}

-(BOOL) videoBandwidthDownMax:(int*)bandwidth {
    if (jsonConfig["video_bandwidth_down_max"].isNumeric()) {
        *bandwidth = jsonConfig["video_bandwidth_down_max"].asInt();
        return YES;
    }
    return NO;
}

-(BOOL) videoMotionRank:(int*)motionRank {
    if (jsonConfig["video_motion_rank"].isNumeric()) {
        *motionRank = jsonConfig["video_motion_rank"].asInt();
        return YES;
    }
    return NO;
}

-(BOOL) videoCongestionCtrlEnabled:(BOOL*)enabled {
    if (jsonConfig["video_congestion_ctrl_enabled"].isBool()) {
        *enabled = jsonConfig["video_congestion_ctrl_enabled"].asBool();
        return YES;
    }
    return NO;
}

-(BOOL) videoJbEnabled:(BOOL*)enabled {
    if (jsonConfig["video_jb_enabled"].isBool()) {
        *enabled = jsonConfig["video_jb_enabled"].asBool();
        return YES;
    }
    return NO;
}

-(BOOL) videoAvpfTail:(int*)min_ max:(int*)max_ {
    if (jsonConfig["video_avpf_tail"].isString()) {
        char min[24], max[24];
        SC_ASSERT(sscanf(jsonConfig["video_avpf_tail"].asCString(), "%23s %23s", min, max) != EOF);
        *min_ = atoi(min);
        *max_ = atoi(max);
        return YES;
    }
    return NO;
}

-(BOOL) videoZeroArtifactsEnabled:(BOOL*)enabled {
    if (jsonConfig["video_zeroartifacts_enabled"].isBool()) {
        *enabled = jsonConfig["video_zeroartifacts_enabled"].asBool();
        return YES;
    }
    return NO;
}

-(BOOL) audioEchoSuppEnabled:(BOOL*)enabled {
    if (jsonConfig["audio_echo_supp_enabled"].isBool()) {
        *enabled = jsonConfig["audio_echo_supp_enabled"].asBool();
        return YES;
    }
    return NO;
}

-(BOOL) audioEchoTail:(int*)tailLength {
    if (jsonConfig["audio_echo_tail"].isNumeric()) {
        *tailLength = jsonConfig["audio_echo_tail"].asInt();
        return NO;
    }
    return NO;
}

-(BOOL) sslCertificates:(NSString**)strPublicKey privateKey:(NSString**)strPrivateKey CA:(NSString**)strCA {
    *strPublicKey = jsonConfig["ssl_file_pub"].isString() ? toNSString(jsonConfig["ssl_file_pub"].asCString()) : @"SSL_Pub.pem";
    *strPrivateKey = jsonConfig["ssl_file_priv"].isString() ? toNSString(jsonConfig["ssl_file_priv"].asCString()) : @"SSL_Priv.pem";
    *strCA = jsonConfig["ssl_file_ca"].isString() ? toNSString(jsonConfig["ssl_file_ca"].asCString()) : @"SSL_CA.pem";
    return YES;
}

-(BOOL) nattIceServersCount:(NSUInteger*)count {
    *count = (jsonConfig["natt_ice_servers"].isArray() && jsonConfig["natt_ice_servers"].size() > 0) ? jsonConfig["natt_ice_servers"].size() : 0;
    return YES;
}

-(BOOL) nattIceServersAt:(NSUInteger)index server:(id<SCObjcIceServer>*)iceServer {
    NSUInteger count;
    if (![self nattIceServersCount:&count]) {
        return NO;
    }
    if (count <= index) {
        return NO;
    }
    *iceServer = [[[IceServer alloc] init] autorelease];
    ((IceServer*)*iceServer)->protocol = toNSString(jsonConfig["natt_ice_servers"][(Json::ArrayIndex)index]["protocol"].asCString());
    ((IceServer*)*iceServer)->host = toNSString(jsonConfig["natt_ice_servers"][(Json::ArrayIndex)index]["host"].asCString());
    ((IceServer*)*iceServer)->port = jsonConfig["natt_ice_servers"][(Json::ArrayIndex)index]["port"].asUInt();
    ((IceServer*)*iceServer)->turnEnabled = jsonConfig["natt_ice_servers"][(Json::ArrayIndex)index]["enable_turn"].asBool();
    ((IceServer*)*iceServer)->stunEnabled = jsonConfig["natt_ice_servers"][(Json::ArrayIndex)index]["enable_stun"].asBool();
    ((IceServer*)*iceServer)->login = toNSString(jsonConfig["natt_ice_servers"][(Json::ArrayIndex)index]["login"].asCString());
    ((IceServer*)*iceServer)->password = toNSString(jsonConfig["natt_ice_servers"][(Json::ArrayIndex)index]["password"].asCString());
    return YES;
}

-(BOOL) nattIceStunEnabled:(BOOL*)enabled {
    if (jsonConfig["natt_ice_stun_enabled"].isBool()) {
        *enabled = jsonConfig["natt_ice_stun_enabled"].asBool();
        return YES;
    }
    return NO;
}

-(BOOL) nattIceTurnEnabled:(BOOL*)enabled {
    if (jsonConfig["natt_ice_turn_enabled"].isBool()) {
        *enabled = jsonConfig["natt_ice_turn_enabled"].asBool();
        return YES;
    }
    return NO;
}

-(BOOL) webproxyAutoDetect:(BOOL*)autodetect {
    if (jsonConfig["webproxy_discovery_auto_enabled"].isBool()) {
        *autodetect = jsonConfig["webproxy_discovery_auto_enabled"].asBool();
        return YES;
    }
    return NO;
}

-(BOOL) webproxyInfo:(NSString**)strType host:(NSString**)strHost port:(unsigned short*)iPort login:(NSString**)strLogin password:(NSString**)strPassword
{
    Json::Value webproxy = jsonConfig["webproxy"];
    if (!webproxy.isNull() && webproxy.isObject()) {
        *strType = webproxy["type"].isString() ? toNSString(webproxy["type"].asCString()) : NULL;
        *strHost = webproxy["host"].isString() ? toNSString(webproxy["host"].asCString()) : NULL;
        *iPort = webproxy["port"].isNumeric() ? webproxy["port"].asInt() : 0;
        *strLogin = webproxy["login"].isString() ? toNSString(webproxy["login"].asCString()) : NULL;
        *strPassword = webproxy["password"].isString() ? toNSString(webproxy["password"].asCString()) : NULL;
        return YES;
    }
    return NO;
}

@end

//
//  Engine
//
@interface SCObjcEngine(Private)
-(void)dummyCoCoaThread;
@end

@implementation SCObjcEngine
-(void)dummyCoCoaThread {
    SC_DEBUG_INFO_EX(kTAG, "dummyCoCoaThread()");
}

+(BOOL) initWithUserId:(const NSString*)userId andPassword:(const NSString*)password {
    if (!SCEngine::isInitialized()) {
        /* http://developer.apple.com/library/mac/#documentation/Cocoa/Reference/Foundation/Classes/NSAutoreleasePool_Class/Reference/Reference.html
         Note: If you are creating secondary threads using the POSIX thread APIs instead of NSThread objects, you cannot use Cocoa, including NSAutoreleasePool, unless Cocoa is in multithreading mode.
         Cocoa enters multithreading mode only after detaching its first NSThread object.
         To use Cocoa on secondary POSIX threads, your application must first detach at least one NSThread object, which can immediately exit.
         You can test whether Cocoa is in multithreading mode with the NSThread class method isMultiThreaded.
         */
        [NSThread detachNewThreadSelector:@selector(dummyCoCoaThread) toTarget:[[[SCObjcEngine alloc] init] autorelease] withObject:nil];
        if ([NSThread isMultiThreaded]) {
            SC_DEBUG_INFO_EX(kTAG, "Working in multithreaded mode :)");
        }
        else {
            SC_DEBUG_WARN_EX(kTAG, "NOT working in multithreaded mode :(");
        }
    }
    return SCEngine::init(toStdString(userId), toStdString(password));
}

+(BOOL) initWithUserId:(const NSString*)userId {
    return [SCObjcEngine initWithUserId:userId andPassword:nil];
}

+(BOOL) setDebugLevel:(SCObjcDebugLevel)eLevel{
    return SCEngine::setDebugLevel((SCDebugLevel_t) eLevel);
}

+(BOOL) setSSLCertificates:(const NSString*)strPublicKey privateKey:(const NSString*)strPrivateKey CA:(const NSString*)strCA mutualAuth:(BOOL)bMutualAuth {
    return SCEngine::setSSLCertificates(toCString(strPublicKey), toCString(strPrivateKey), toCString(strCA), bMutualAuth);
}

+(BOOL) setSSLCertificates:(const NSString*)strPublicKey privateKey:(const NSString*)strPrivateKey CA:(const NSString*)strCA {
    return [SCObjcEngine setSSLCertificates:strPublicKey privateKey:strPrivateKey CA:strCA mutualAuth: FALSE];
}

+(BOOL) setVideoPrefSize:(const NSString*)strPrefVideoSize {
    return SCEngine::setVideoPrefSize(toCString(strPrefVideoSize));
}

+(BOOL) setVideoFps:(int)fps {
    return SCEngine::setVideoFps(fps);
}

+(BOOL) setVideoBandwidthUpMax:(int)bandwidthMax {
    return SCEngine::setVideoBandwidthUpMax(bandwidthMax);
}

+(BOOL) setVideoBandwidthDownMax:(int)bandwidthMax {
    return SCEngine::setVideoBandwidthDownMax(bandwidthMax);
}

+(BOOL) setVideoMotionRank:(int)motionRank {
    return SCEngine::setVideoMotionRank(motionRank);
}

+(BOOL) setVideoCongestionCtrlEnabled:(BOOL)congestionCtrl {
    return SCEngine::setVideoCongestionCtrlEnabled(congestionCtrl);
}

+(BOOL) setVideoJbEnabled:(BOOL)enabled {
    return SCEngine::setVideoJbEnabled(enabled);
}

+(BOOL) setVideoAvpfTail:(int)min max:(int)max {
    return SCEngine::setVideoAvpfTail(min, max);
}

+(BOOL) setVideoZeroArtifactsEnabled:(BOOL)enabled {
    return SCEngine::setVideoZeroArtifactsEnabled(enabled);
}

+(BOOL) setAudioEchoSuppEnabled:(BOOL)enabled {
#if HAVE_COREAUDIO_AUDIO_UNIT && TARGET_OS_IPHONE // iOS devices have native AEC
    if (!enabled) {
        SC_DEBUG_WARN_EX(kTAG, "setAudioEchoSuppEnabled(NO) not available on iOS device. Native AEC always enabled");
        return NO;
    }
    return YES;
#else
    return SCEngine::setAudioEchoSuppEnabled(enabled);
#endif
}

+(BOOL) setAudioEchoTail:(int)tailLength {
    return SCEngine::setAudioEchoTail(tailLength);
}

+(BOOL) addNattIceServer:(const NSString*)strTransportProto  serverHost:(const NSString*)strServerHost serverPort:(unsigned short)iServerPort useTurn:(BOOL)bUseTurn useStun:(BOOL)bUseStun userName:(const NSString*)strUsername password:(const NSString*) strPassword {
    return SCEngine::addNattIceServer(toCString(strTransportProto), toCString(strServerHost), iServerPort, bUseTurn, bUseStun, toCString(strUsername), toCString(strPassword));
}

+(BOOL) addNattIceServer:(const NSString*)strTransportProto  serverHost:(const NSString*)strServerHost serverPort:(unsigned short)iServerPort useTurn:(BOOL)bUseTurn useStun:(BOOL)bUseStun {
    return [SCObjcEngine addNattIceServer:strTransportProto serverHost:strServerHost serverPort:iServerPort useTurn:bUseTurn useStun:bUseStun userName:nil password:nil];
}

+(BOOL) addNattIceServer:(const NSString*)strTransportProto  serverHost:(const NSString*)strServerHost serverPort:(unsigned short)iServerPort {
    return [SCObjcEngine addNattIceServer:strTransportProto serverHost:strServerHost serverPort:iServerPort useTurn:NO useStun:YES userName:nil password:nil];
}

+(BOOL) clearNattIceServers {
    return SCEngine::clearNattIceServers();
}

+(BOOL) setNattIceStunEnabled:(BOOL)enabled {
    return SCEngine::setNattIceStunEnabled(enabled);
}

+(BOOL) setNattIceTurnEnabled:(BOOL)enabled {
    return SCEngine::setNattIceTurnEnabled(enabled);
}

+(BOOL) setWebProxyAutodetect:(BOOL)autodetect{
    return SCEngine::setWebProxyAutodetect(autodetect);
}

+(BOOL) setWebProxyInfo:(const NSString*)strType host:(const NSString*)strHost port:(unsigned short)iPort login:(const NSString*)strLogin password:(const NSString*)strPassword {
    return SCEngine::setWebProxyInfo(toCString(strType), toCString(strHost), iPort, toCString(strLogin), toCString(strPassword));
}

@end

//
//  SignalingCallEvent
//
@implementation SignalingCallEvent
+(id<SCObjcSignalingCallEvent>) createWithEvent:(SCObjWrapper<SCSignalingCallEvent* >)event {
    SC_ASSERT(event);
    SignalingCallEvent* event_ = [[[SignalingCallEvent alloc] init] autorelease];
    SC_ASSERT(event_);
    event_->event = event;
    return event_;
}
-(NSString*) type {
    return toNSString(event->getType());
}
-(NSString*) from {
    return toNSString(event->getFrom());
}
-(NSString*) to {
    return toNSString(event->getTo());
}
-(NSString*) callID {
    return toNSString(event->getCallId());
}
-(NSString*) transactionID {
    return toNSString(event->getTransacId());
}
-(NSString*) sdp {
    return toNSString(event->getSdp());
}
-(void)dealloc {
    event = NULL;
    [super dealloc];
}
@end

//
//  Session
//
@implementation Session
-(Session*) initWithType:(SCObjcSessionType)type_ signaling:(id<SCObjcSignaling>)signaling_ {
    SC_ASSERT(self = [super init]);
    SC_ASSERT(signaling_ != nil);
    SC_ASSERT([signaling_ class] == [Signaling class]);
    type = type_;
    signaling = signaling_;
    return self;
}
-(SCObjcSessionType) type {
    return type;
}
+(SCObjcMediaType) mediaTypeFromNative:(SCMediaType_t)type_ {
    if (type_ == SCMediaType_All) return SCObjcMediaTypeAll;
    SCObjcMediaType type = SCObjcMediaTypeNone;
    if (type_ & SCMediaType_Audio) type = (SCObjcMediaType)(type| SCObjcMediaTypeAudio);
    if (type_ & SCMediaType_Video) type = (SCObjcMediaType)(type| SCObjcMediaTypeVideo);
    if (type_ & SCMediaType_ScreenCast) type = (SCObjcMediaType)(type| SCObjcMediaTypeScreenCast);
    return type;
}
+(SCMediaType_t) mediaTypeToNative:(SCObjcMediaType)type_ {
    if (type_ == SCObjcMediaTypeAll) return SCMediaType_All;
    SCMediaType_t type = SCMediaType_None;
    if (type_ & SCObjcMediaTypeAudio) type = (SCMediaType_t)(type| SCMediaType_Audio);
    if (type_ & SCObjcMediaTypeVideo) type = (SCMediaType_t)(type| SCMediaType_Video);
    if (type_ & SCObjcMediaTypeScreenCast) type = (SCMediaType_t)(type| SCMediaType_ScreenCast);
    return type;
}
-(void)dealloc {
    signaling = NULL;
    [super dealloc];
}
@end

@implementation SessionCall
-(SessionCall*)initWithSignaling:(id<SCObjcSignaling>)signaling_ offer:(id<SCObjcSignalingCallEvent>)e {
    SC_ASSERT(self = [super initWithType:SCObjcSessionTypeCall signaling:signaling_]);
    SC_ASSERT(signaling_);
    SC_ASSERT([signaling_ class] == [Signaling class]);
    if (e) {
        SC_ASSERT([e class] == [SignalingCallEvent class]);
        SC_ASSERT((call = SCSessionCall::newObj(((Signaling*)signaling_)->signalSession, ((SignalingCallEvent*)e)->event)));
    }
    else {
        SC_ASSERT((call = SCSessionCall::newObj(((Signaling*)signaling_)->signalSession)));
    }
    
    if ([[[UIDevice currentDevice] systemVersion] doubleValue] >= 6.0) {
        [[NSNotificationCenter defaultCenter]
         addObserver:self selector:@selector(onAudioSessionInterruptionEvent:) name:AVAudioSessionInterruptionNotification object:[AVAudioSession sharedInstance]];
    }
#if __IPHONE_OS_VERSION_MIN_REQUIRED < 6000
    else {
        [[AVAudioSession sharedInstance] setDelegate:self]; // deprecated starting SDK6
    }
#endif
    
    return self;
}

-(SessionCall*)initWithSignaling:(id<SCObjcSignaling>)signaling_ {
    return [self initWithSignaling:signaling_ offer:nil];
}

-(void)onAudioSessionInterruptionEvent:(NSNotification*)notif {
    const NSInteger iType = [[[notif userInfo] valueForKey:AVAudioSessionInterruptionTypeKey] intValue];
    SC_DEBUG_INFO_EX(kTAG, "onAudioSessionInterruptionEvent:%d", (int)iType);
    switch (iType) {
        case AVAudioSessionInterruptionTypeBegan: {
            call->setAudioInterrupt(true);
            if (delegate && [delegate respondsToSelector:@selector(callInterruptionChanged:)]) {
                [delegate callInterruptionChanged:YES];
            }
            break;
        }
        case AVAudioSessionInterruptionTypeEnded: {
            call->setAudioInterrupt(false);
            if (delegate && [delegate respondsToSelector:@selector(callInterruptionChanged:)]) {
                [delegate callInterruptionChanged:NO];
            }
            break;
        }
    }
}

#if __IPHONE_OS_VERSION_MIN_REQUIRED < 6000
-(void)beginInterruption {
    SC_DEBUG_INFO_EX(kTAG, "beginInterruption");
    call->setAudioInterrupt(true);
    if (delegate && [delegate respondsToSelector:@selector(callInterruptionChanged:)]) {
        [delegate callInterruptionChanged:YES];
    }
}

-(void)endInterruption {
    SC_DEBUG_INFO_EX(kTAG, "endInterruption");
    call->setAudioInterrupt(false);
    if (delegate && [delegate respondsToSelector:@selector(callInterruptionChanged:)]) {
        [delegate callInterruptionChanged:NO];
    }
}
#endif /* __IPHONE_OS_VERSION_MIN_REQUIRED < 6000 */

-(BOOL) setDelegate:(id<SCObjcSessionCallDelegate>)delegate_ {
    delegate = delegate_;
    return call->setIceCallback(SCSessionCallIceCallbackDummy::newObj(delegate));
}

-(BOOL) call:(SCObjcMediaType)mediaType destinationID:(const NSString*)strDestinationID {
    if (!strDestinationID) {
        SC_DEBUG_ERROR_EX(kTAG, "Invalid parameter");
        return NO;
    }
    return call->call([Session mediaTypeToNative:mediaType], toCString(strDestinationID));
}

-(BOOL) reset {
    return call->sessionMgrReset();
}

-(BOOL) isReady {
    return call->sessionMgrIsReady();
}

-(BOOL) start {
    return call->sessionMgrStart();
}

-(BOOL) stop {
    return call->sessionMgrStop();
}

-(BOOL) pause {
    return call->sessionMgrPause();
}

-(BOOL) resume {
    return call->sessionMgrResume();
}

-(BOOL) acceptEvent:(id<SCObjcSignalingCallEvent>)e {
    SC_ASSERT(e);
    SC_ASSERT([e class] == [SignalingCallEvent class]);
    return call->acceptEvent(((SignalingCallEvent*)e)->event);
}

-(BOOL) rejectEvent:(id<SCObjcSignalingCallEvent>)e {
    SC_ASSERT(e);
    SC_ASSERT([e class] == [SignalingCallEvent class]);
    return SCSessionCall::rejectEvent(signaling->signalSession, ((SignalingCallEvent*)e)->event);
}

-(BOOL) setMute:(BOOL)bMuted mediaType:(SCObjcMediaType)eMediaType {
    return call->setMute(bMuted, [Session mediaTypeToNative:eMediaType]);
}

-(BOOL) setMute:(BOOL)bMuted {
    return [self setMute:bMuted mediaType:SCObjcMediaTypeAll];
}

-(BOOL) toggleCamera {
    return call->toggleCamera();
}

-(BOOL) useFrontCamera:(BOOL)bFront {
    return call->useFrontCamera(bFront);
}

-(BOOL) useFrontCamera {
    return [self useFrontCamera:YES];
}

-(BOOL) useBackCamera {
    return [self useFrontCamera:NO];
}

-(BOOL) hangup {
    return call->hangup();
}

-(BOOL) setVideoDisplays:(SCObjcMediaType)eVideoType local:(SCObjcVideoDisplayLocal)localDisplay remote:(SCObjcVideoDisplayRemote)remoteDisplay {
    return call->setVideoDisplays([Session mediaTypeToNative:eVideoType], localDisplay, remoteDisplay);
}

-(BOOL) setVideoDisplays:(SCObjcMediaType)eVideoType local:(SCObjcVideoDisplayLocal)localDisplay {
    return [self setVideoDisplays:eVideoType local:localDisplay remote:nil];
}

-(BOOL) setVideoDisplays:(SCObjcMediaType)eVideoType remote:(SCObjcVideoDisplayRemote)remoteDisplay {
    return [self setVideoDisplays:eVideoType local:nil remote:remoteDisplay];
}

-(BOOL) setVideoFps:(int)fps mediaType:(SCObjcMediaType)eMediaType {
    return call->setVideoFps(fps, [Session mediaTypeToNative:eMediaType]);
}

-(BOOL) setVideoFps:(int)fps {
    return [self setVideoFps:fps mediaType:(SCObjcMediaType)(SCObjcMediaTypeVideo | SCObjcMediaTypeScreenCast)];
}

-(BOOL) setVideoBandwidthUploadMax:(int)maxBandwidth mediaType:(SCObjcMediaType)eMediaType {
    return call->setVideoBandwidthUploadMax(maxBandwidth, [Session mediaTypeToNative:eMediaType]);
}

-(BOOL) setVideoBandwidthUploadMax:(int)maxBandwidth {
    return [self setVideoBandwidthUploadMax:maxBandwidth mediaType:(SCObjcMediaType)(SCObjcMediaTypeVideo | SCObjcMediaTypeScreenCast)];
}

-(BOOL) setVideoBandwidthDownloadMax:(int)maxBandwidth mediaType:(SCObjcMediaType)eMediaType {
    return call->setVideoBandwidthDownloadMax(maxBandwidth, [Session mediaTypeToNative:eMediaType]);
}

-(BOOL) setVideoBandwidthDownloadMax:(int)maxBandwidth {
    return [self setVideoBandwidthDownloadMax:maxBandwidth mediaType:(SCObjcMediaType)(SCObjcMediaTypeVideo | SCObjcMediaTypeScreenCast)];
}

-(BOOL) sendData:(NSData*)data {
    if (data) {
        return call->sendData(data.bytes, data.length);
    }
    return NO;
}

-(BOOL) sendFreezeFrame:(BOOL)freeze {
    Json::Value root;
    root["messageType"] = "command";
    root["passthrough"] = YES;
    root["action"] = freeze ? "freezeFrame" : "liveStream";
    std::string json = root.toStyledString();
    return call->sendData(json.c_str(), json.length());
}

-(BOOL) sendClearAnnotations {
    Json::Value root;
    root["messageType"] = "command";
    root["passthrough"] = YES;
    root["action"] = "clearAllAnnotation";
    std::string json = root.toStyledString();
    return call->sendData(json.c_str(), json.length());
}

-(NSString*) callID {
    return toNSString(call->getCallId());
}
-(SCObjcMediaType) mediaType {
    return [Session mediaTypeFromNative:call->getMediaType()];
}

-(SCObjcIceState) iceState {
    switch (call->getIceState()) {
        case SCIceState_None: return SCObjcIceStateNone;
        case SCIceState_Failed: return SCObjcIceStateFailed;
        case SCIceState_GatheringDone: return SCObjcIceStateGatheringDone;
        case SCIceState_Connected: return SCObjcIceStateConnected;
        case SCIceState_Teminated: return SCObjcIceStateTeminated;
    }
}

-(void)dealloc {
    call = NULL;
    delegate = nil;
    [super dealloc];
    
    SC_DEBUG_INFO_EX(kTAG, "*** SessionCall destroyed ***");
}
@end

//
//  Signaling
//
@implementation Signaling

-(Signaling*) initWithConnectionUrl:(const NSString*)connectionUrl localIP:(const NSString *)strLocalIP localPort:(const unsigned short)uLocalPort {
    SC_ASSERT(self = [super init]);
    signalSession = SCSignaling::newObj(toCString(connectionUrl), toCString(strLocalIP), uLocalPort);
    if (!signalSession) {
        return NULL;
    }
    
    return self;
}

-(BOOL) setDelegate:(id<SCObjcSignalingDelegate>)delegate_ {
    return signalSession->setCallback(SCSignalingCallbackDummy::newObj(delegate_));
}

-(BOOL) isConnected {
    return signalSession->isConnected();
}

-(BOOL) isReady {
    return signalSession->isReady();
}

-(BOOL) connect {
    return signalSession->connect();
}

-(BOOL) sendData:(const NSData*)data {
    if (data) {
        return signalSession->sendData(data.bytes, data.length);
    }
    return NO;
}

-(BOOL) sendChatMessage:(const NSString*)message username:(const NSString*)strUsername {
    if (strUsername && message) {
        Json::Value root;
        root["messageType"] = "chat";
        root["username"] = toCString(strUsername);
        root["message"] = toCString(message);
        root["passthrough"] = YES;
        std::string json = root.toStyledString();
        return signalSession->sendData(json.c_str(), json.length());
    }
    return NO;
}

-(BOOL) disConnect {
    if (signalSession) {
        return signalSession->disConnect();
    }
    return NO;
}

-(BOOL) rejectEventCall:(id<SCObjcSignalingCallEvent>)e_ {
    SC_ASSERT(e_);
    SC_ASSERT([e_ class] == [SignalingCallEvent class]);
    return SCSessionCall::rejectEvent(signalSession, ((SignalingCallEvent*)e_)->event);
}

-(void)dealloc {
    signalSession = NULL;
    [super dealloc];
    SC_DEBUG_INFO_EX(kTAG, "*** Signaling destroyed ***");
}

@end

//
//  SCObjcFactory
//
@implementation SCObjcFactory
+(id<SCObjcSignaling>) createSignalingWithConnectionUrl:(const NSString*)connectionUrl localIP:(const NSString *)strLocalIP localPort:(const unsigned short)uLocalPort {
    return [[[Signaling alloc] initWithConnectionUrl:connectionUrl localIP:strLocalIP localPort:uLocalPort] autorelease];
}
+(id<SCObjcSignaling>) createSignalingWithConnectionUrl:(const NSString*)connectionUrl localIP:(const NSString *)strLocalIP {
    return [SCObjcFactory createSignalingWithConnectionUrl:connectionUrl localIP:strLocalIP localPort:0];
}
+(id<SCObjcSignaling>) createSignalingWithConnectionUrl:(const NSString*)connectionUrl {
    return [SCObjcFactory createSignalingWithConnectionUrl:connectionUrl localIP:NULL localPort:0];
}
+(id<SCObjcConfig>) createConfigWithFile:(const NSString*)fullPath {
    return [[[Config alloc] initWithFile:fullPath] autorelease];
}
+(id<SCObjcSessionCall>) createCallWithSignaling:(id<SCObjcSignaling>)signaling_ offer:(id<SCObjcSignalingCallEvent>)e_ {
    SC_ASSERT(signaling_ != nil);
    return [[[SessionCall alloc] initWithSignaling:signaling_ offer:e_] autorelease];
}
+(id<SCObjcSessionCall>) createCallWithSignaling:(id<SCObjcSignaling>)signaling_ {
    return [SCObjcFactory createCallWithSignaling:signaling_ offer:nil];
}
@end