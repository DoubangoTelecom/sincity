#ifndef SINCITY_API_OBJC_H
#define SINCITY_API_OBJC_H

#import <UIKit/UIKit.h>
#import <TargetConditionals.h>

#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
#   import "Sincity/sc_glview_ios.h"
#   define SCObjcVideoDisplayLocal UIView*
#   define SCObjcVideoDisplayRemote SCGlviewIOS*
#else
#   define SCObjcVideoDisplayLocal UIView*
#   define SCObjcVideoDisplayRemote UIView*
#endif

#define SCNSLog(TAG, FMT, ...) NSLog(@"%@" FMT "\n", TAG, ##__VA_ARGS__)

#define kSCOjcSessionCallTypeHangup @"hangup"
#define kSCOjcSessionCallTypeOffer @"offer"
#define kSCOjcSessionCallTypeAnswer @"answer"
#define kSCOjcSessionCallTypeProvisional @"pr-answer"

#ifndef NS_ENUM
#   define NS_ENUM(_type, _name) enum _name : _type _name; enum _name : _type
#endif

@protocol SCObjcSignalingCallEvent;

// ==SCObjcDebugLevel==
typedef enum {
    SCObjcDebugLevelInfo = 4,
    SCObjcDebugLevelWarn = 3,
    SCObjcDebugLevelError = 2,
    SCObjcDebugLevelFatal = 1
}
SCObjcDebugLevel;

// ==SCObjcMediaType==
typedef NS_ENUM(NSInteger, SCObjcMediaType) {
    SCObjcMediaTypeNone = 0x00,
    SCObjcMediaTypeAudio = (0x01<<0),
    SCObjcMediaTypeVideo = (0x01<<1),
    SCObjcMediaTypeScreenCast = (0x01<<2),
    SCObjcMediaTypeAudioVideo = (SCObjcMediaTypeAudio | SCObjcMediaTypeVideo),
    SCObjcMediaTypeAll = 0xFF,
};

// ==SCObjcIceState==
typedef NS_ENUM(NSInteger, SCObjcIceState) {
    SCObjcIceStateNone,
    SCObjcIceStateFailed,
    SCObjcIceStateGatheringDone,
    SCObjcIceStateConnected,
    SCObjcIceStateTeminated
};

// ==SCObjcSessionType==
typedef NS_ENUM(NSInteger, SCObjcSessionType) {
    SCObjcSessionTypeNone,
    SCObjcSessionTypeCall
};

// ==SCObjcViewport==
@protocol SCObjcViewport <NSObject>
@property(readonly) int x;
@property(readonly) int y;
@property(readonly) int width;
@property(readonly) int height;
@end

// ==SCObjcIceServer==
@protocol SCObjcIceServer <NSObject>
@required
@property(readonly) NSString* protocol;
@property(readonly) NSString* host;
@property(readonly) unsigned short port;
@property(readonly) BOOL turnEnabled;
@property(readonly) BOOL stunEnabled;
@property(readonly) NSString* login;
@property(readonly) NSString* password;
@end

// ==SCObjcConfig==
@protocol SCObjcConfig <NSObject>
@required
-(BOOL) debugLevel:(SCObjcDebugLevel*)level;
-(BOOL) localID:(NSString**)strId;
-(BOOL) remoteID:(NSString**)strId;
-(BOOL) connectionUrl:(NSString**)strUrl;
-(BOOL) sslCertificates:(NSString**)strPublicKey privateKey:(NSString**)strPrivateKey CA:(NSString**)strCA;
-(BOOL) videoPrefSize:(NSString**)strPrefSize;
-(BOOL) videoFps:(int*)fps;
-(BOOL) videoBandwidthUpMax:(int*)bandwidth;
-(BOOL) videoBandwidthDownMax:(int*)bandwidth;
-(BOOL) videoMotionRank:(int*)motionRank;
-(BOOL) videoCongestionCtrlEnabled:(BOOL*)enabled;
-(BOOL) videoJbEnabled:(BOOL*)enabled;
-(BOOL) videoAvpfTail:(int*)min max:(int*)max;
-(BOOL) videoZeroArtifactsEnabled:(BOOL*)enabled;
-(BOOL) audioEchoSuppEnabled:(BOOL*)enabled;
-(BOOL) audioEchoTail:(int*)tailLength;
-(BOOL) nattIceServersCount:(NSUInteger*)count;
-(BOOL) nattIceServersAt:(NSUInteger)index server:(id<SCObjcIceServer>*)iceServer;
-(BOOL) nattIceStunEnabled:(BOOL*)enabled;
-(BOOL) nattIceTurnEnabled:(BOOL*)enabled;
-(BOOL) webproxyAutoDetect:(BOOL*)autodetect;
-(BOOL) webproxyInfo:(NSString**)strType host:(NSString**)strHost port:(unsigned short*)iPort login:(NSString**)strLogin password:(NSString**)strPassword;
@end

// ==SCObjcEngine==
@interface SCObjcEngine : NSObject {
}
+(BOOL) initWithUserId:(const NSString*)userId andPassword:(const NSString*)password;
+(BOOL) initWithUserId:(const NSString*)userId;
+(BOOL) setDebugLevel:(SCObjcDebugLevel)eLevel;
+(BOOL) setSSLCertificates:(const NSString*)strPublicKey privateKey:(const NSString*)strPrivateKey CA:(const NSString*)strCA mutualAuth:(BOOL)bMutualAuth;
+(BOOL) setSSLCertificates:(const NSString*)strPublicKey privateKey:(const NSString*)strPrivateKey CA:(const NSString*)strCA;
+(BOOL) setVideoPrefSize:(const NSString*)strPrefVideoSize;
+(BOOL) setVideoFps:(int)fps;
+(BOOL) setVideoBandwidthUpMax:(int)bandwidthMax;
+(BOOL) setVideoBandwidthDownMax:(int)bandwidthMax;
+(BOOL) setVideoMotionRank:(int)motionRank;
+(BOOL) setVideoCongestionCtrlEnabled:(BOOL)congestionCtrl;
+(BOOL) setVideoJbEnabled:(BOOL)enabled;
+(BOOL) setVideoAvpfTail:(int)min max:(int)max;
+(BOOL) setVideoZeroArtifactsEnabled:(BOOL)enabled;
+(BOOL) setAudioEchoSuppEnabled:(BOOL)enabled;
+(BOOL) setAudioEchoTail:(int)tailLength;
+(BOOL) addNattIceServer:(const NSString*)strTransportProto  serverHost:(const NSString*)strServerHost serverPort:(unsigned short)iServerPort useTurn:(BOOL)bUseTurn useStun:(BOOL)bUseStun userName:(const NSString*)strUsername password:(const NSString*) strPassword;
+(BOOL) addNattIceServer:(const NSString*)strTransportProto  serverHost:(const NSString*)strServerHost serverPort:(unsigned short)iServerPort useTurn:(BOOL)bUseTurn useStun:(BOOL)bUseStun;
+(BOOL) addNattIceServer:(const NSString*)strTransportProto  serverHost:(const NSString*)strServerHost serverPort:(unsigned short)iServerPort;
+(BOOL) clearNattIceServers;
+(BOOL) setNattIceStunEnabled:(BOOL)enabled;
+(BOOL) setNattIceTurnEnabled:(BOOL)enabled;
+(BOOL) setWebProxyAutodetect:(BOOL)autodetect;
+(BOOL) setWebProxyInfo:(const NSString*)strType host:(const NSString*)strHost port:(unsigned short)iPort login:(const NSString*)strLogin password:(const NSString*)strPassword;
@end

// ==SCObjcSignalingDelegate==
@protocol SCObjcSignalingDelegate <NSObject>
@optional
-(BOOL) signalingDidConnect:(const NSString*)description;
-(BOOL) signalingDidDisconnect:(const NSString*)description;
-(BOOL) signalingGotData:(const NSData*)data;
-(BOOL) signalingGotEventCall:(id<SCObjcSignalingCallEvent>)event;
-(BOOL) signalingGotChatMessage:(const NSString*)message username:(const NSString*)strUsername;
@end

// ==SCObjcSignaling==
@protocol SCObjcSignaling <NSObject>
@required
-(BOOL) setDelegate:(id<SCObjcSignalingDelegate>)delegate;
-(BOOL) isConnected;
@property(readonly, getter=isConnected) BOOL connected;
-(BOOL) isReady;
@property(readonly, getter=isReady) BOOL ready;
-(BOOL) connect;
-(BOOL) sendData:(const NSData*)data;
-(BOOL) sendChatMessage:(const NSString*)message username:(const NSString*)strUsername;
-(BOOL) disConnect;
-(BOOL) rejectEventCall:(id<SCObjcSignalingCallEvent>)e;
@end

// ==SCObjcSignalingCallEvent==
@protocol SCObjcSignalingCallEvent <NSObject>
@required
@property(readonly) NSString* type; // The event type. e.g. "offer", "answer", "hangup"...
@property(readonly) NSString* from; // The source identifier
@property(readonly) NSString* to; // The destination identifier
@property(readonly) NSString* callID; // The call identifier
@property(readonly) NSString* transactionID; // The transaction identifier
@property(readonly) NSString* sdp; // The session description. Could be NULL.
@property(readonly) NSString* description; // The event description
@end

// ==SCObjcSessionCallDelegate==
@protocol SCObjcSessionCallDelegate <NSObject>
@optional
-(BOOL) callIceStateChanged;
-(BOOL) callInterruptionChanged:(BOOL)interrupted;
@end

// ==SCObjcSession==
@protocol SCObjcSession <NSObject>
@required
@property(readonly) SCObjcSessionType type;
@end

// ==SCObjcSessionCall==
@protocol SCObjcSessionCall <SCObjcSession>
@required
-(BOOL) setDelegate:(id<SCObjcSessionCallDelegate>)delegate;
-(BOOL) call:(SCObjcMediaType)mediaType destinationID:(const NSString*)strDestinationID;
-(BOOL) reset;
-(BOOL) isReady;
@property(readonly, getter=isReady) BOOL ready;
-(BOOL) start;
-(BOOL) stop;
-(BOOL) pause;
-(BOOL) resume;
-(BOOL) acceptEvent:(id<SCObjcSignalingCallEvent>)e;
-(BOOL) rejectEvent:(id<SCObjcSignalingCallEvent>)e;
-(BOOL) setMute:(BOOL)bMuted mediaType:(SCObjcMediaType)eMediaType;
-(BOOL) setMute:(BOOL)bMuted;
-(BOOL) toggleCamera;
-(BOOL) useFrontCamera:(BOOL)bFront;
-(BOOL) useFrontCamera;
-(BOOL) useBackCamera;
-(BOOL) hangup;
-(BOOL) setVideoDisplays:(SCObjcMediaType)eVideoType local:(SCObjcVideoDisplayLocal)localDisplay remote:(SCObjcVideoDisplayRemote)remoteDisplay;
-(BOOL) setVideoDisplays:(SCObjcMediaType)eVideoType local:(SCObjcVideoDisplayLocal)localDisplay;
-(BOOL) setVideoDisplays:(SCObjcMediaType)eVideoType remote:(SCObjcVideoDisplayRemote)remoteDisplay;
-(BOOL) setVideoFps:(int)fps mediaType:(SCObjcMediaType)eMediaType;
-(BOOL) setVideoFps:(int)fps;
-(BOOL) setVideoBandwidthUploadMax:(int)maxBandwidth mediaType:(SCObjcMediaType)eMediaType;
-(BOOL) setVideoBandwidthUploadMax:(int)maxBandwidth;
-(BOOL) setVideoBandwidthDownloadMax:(int)maxBandwidth mediaType:(SCObjcMediaType)eMediaType;
-(BOOL) setVideoBandwidthDownloadMax:(int)maxBandwidth;
-(BOOL) sendData:(NSData*)data;
-(BOOL) sendFreezeFrame:(BOOL)freeze;
-(BOOL) sendClearAnnotations;
@property(readonly) NSString* callID;
@property(readonly) SCObjcMediaType mediaType;
@property(readonly) SCObjcIceState iceState;
@end

// ==SCObjcFactory==
@interface SCObjcFactory : NSObject {
}
+(id<SCObjcSignaling>) createSignalingWithConnectionUrl:(const NSString*)connectionUrl localIP:(const NSString *)strLocalIP localPort:(const unsigned short)uLocalPort;
+(id<SCObjcSignaling>) createSignalingWithConnectionUrl:(const NSString*)connectionUrl localIP:(const NSString *)strLocalIP;
+(id<SCObjcSignaling>) createSignalingWithConnectionUrl:(const NSString*)connectionUr;
+(id<SCObjcConfig>) createConfigWithFile:(const NSString*)fullPath;
+(id<SCObjcSessionCall>) createCallWithSignaling:(id<SCObjcSignaling>)signaling offer:(id<SCObjcSignalingCallEvent>)e;
+(id<SCObjcSessionCall>) createCallWithSignaling:(id<SCObjcSignaling>)signaling;
@end


#endif /* SINCITY_API_OBJC_H */
