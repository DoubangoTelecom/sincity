//
//  ViewController.m
//  SinTest
//
//  Created by Mamadou DIOP on 22/04/15.
//  Copyright (c) 2015 Doubango Telecom. All rights reserved.
//

#import "ViewController.h"
#import "AppDelegate.h"

#define kTAG @"[SinTest::ViewController]"
#define kRejectIncomingCalls NO

#define kSegmentIndexCircle     0
#define kSegmentIndexArrow      1
#define kSegmentIndexText       2
#define kSegmentIndexFreehand   3

@interface ViewController ()
-(void)showInfo:(NSString*)msg;
-(void)showSuccess:(NSString*)msg;
-(void)showError:(NSString*)msg;
@end

@implementation ViewController {
}

@synthesize buttonConnect;
@synthesize buttonCall;
@synthesize buttonFreeze;
@synthesize buttonClear;
@synthesize labelInfo;
@synthesize localView;
@synthesize segAnnotations;
@synthesize remoteView;

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view, typically from a nib.
    buttonFreeze.tag = 0;
    buttonFreeze.enabled = NO;
    buttonClear.enabled = NO;
    buttonCall.enabled = NO;
    
    NSString *nsString1, *nsString2, *nsString3;
    NSUInteger ui1, ui2;
    int i1, i2;
    BOOL b1;
    
    /*** Engine ***/
    // debug level
    SCObjcDebugLevel debugLevel;
    if ([[AppDelegate sharedInstance].config debugLevel:&debugLevel]) {
        assert([SCObjcEngine setDebugLevel:debugLevel]);
    }
    // init
    assert([[AppDelegate sharedInstance].config localID:&nsString1]);
    assert([SCObjcEngine initWithUserId:nsString1]);
    // ssl certificates
    assert([[AppDelegate sharedInstance].config sslCertificates:&nsString1 privateKey:&nsString2 CA:&nsString3]);
    assert([SCObjcEngine setSSLCertificates:[[NSBundle mainBundle] pathForResource:[[nsString1 lastPathComponent] stringByDeletingPathExtension] ofType:[nsString1 pathExtension]]
                                 privateKey:[[NSBundle mainBundle] pathForResource:[[nsString2 lastPathComponent] stringByDeletingPathExtension] ofType:[nsString2 pathExtension]]
                                         CA:[[NSBundle mainBundle] pathForResource:[[nsString3 lastPathComponent] stringByDeletingPathExtension] ofType:[nsString3 pathExtension]]
            ]);
    // video pref size
    if ([[AppDelegate sharedInstance].config videoPrefSize:&nsString1]) {
        assert([SCObjcEngine setVideoPrefSize:nsString1]);
    }
    // video fps
    if ([[AppDelegate sharedInstance].config videoFps:&i1]) {
        assert([SCObjcEngine setVideoFps:i1]);
    }
    // video bandwidth up max
    if ([[AppDelegate sharedInstance].config videoBandwidthUpMax:&i1]) {
        assert([SCObjcEngine setVideoBandwidthUpMax:i1]);
    }
    // video bandwidth down max
    if ([[AppDelegate sharedInstance].config videoBandwidthDownMax:&i1]) {
        assert([SCObjcEngine setVideoBandwidthDownMax:i1]);
    }
    // video motion rank
    if ([[AppDelegate sharedInstance].config videoMotionRank:&i1]) {
        assert([SCObjcEngine setVideoMotionRank:i1]);
    }
    // video congestion control enabled
    if ([[AppDelegate sharedInstance].config videoCongestionCtrlEnabled:&b1]) {
        assert([SCObjcEngine setVideoCongestionCtrlEnabled:b1]);
    }
    // video jitter buffer enabled
    if ([[AppDelegate sharedInstance].config videoJbEnabled:&b1]) {
        assert([SCObjcEngine setVideoJbEnabled:b1]);
    }
    // video zero artifacts enabled
    if ([[AppDelegate sharedInstance].config videoZeroArtifactsEnabled:&b1]) {
        assert([SCObjcEngine setVideoZeroArtifactsEnabled:b1]);
    }
    // video AVPF tail
    if ([[AppDelegate sharedInstance].config videoAvpfTail:&i1 max:&i2]) {
        assert([SCObjcEngine setVideoAvpfTail:i1 max:i2]);
    }
#if !TARGET_OS_IPHONE // Do not enable Doubango AEC. Native AEC always ON on iOS :)
    // audio echo supp enabled
    if ([[AppDelegate sharedInstance].config audioEchoSuppEnabled:&b1]) {
        assert([SCObjcEngine setAudioEchoSuppEnabled:b1]);
    }
    // audio echo tail
    if ([[AppDelegate sharedInstance].config audioEchoTail:&i1]) {
        assert([SCObjcEngine setAudioEchoTail:i1]);
    }
#endif
    // natt ice servers
    if ([[AppDelegate sharedInstance].config nattIceServersCount:&ui1]) {
        NSObject<SCObjcIceServer>* iceServer;
        for (ui2 = 0; ui2 < ui1; ++ui2) {
            assert([[AppDelegate sharedInstance].config nattIceServersAt:ui2 server:&iceServer]);
            assert([SCObjcEngine addNattIceServer:iceServer.protocol serverHost:iceServer.host serverPort:iceServer.port useTurn:iceServer.turnEnabled useStun:iceServer.stunEnabled userName:iceServer.login password:iceServer.password]);
        }
    }
    // natt ice-stun enabled
    if ([[AppDelegate sharedInstance].config nattIceStunEnabled:&b1]) {
        assert([SCObjcEngine setNattIceStunEnabled:b1]);
    }
    // natt ice-turn enabled
    if ([[AppDelegate sharedInstance].config nattIceTurnEnabled:&b1]) {
        assert([SCObjcEngine setNattIceTurnEnabled:b1]);
    }
    
    /*** signaling ***/
    assert([[AppDelegate sharedInstance].config connectionUrl:&nsString1]);
    assert((signalingSession = [SCObjcFactory createSignalingWithConnectionUrl:nsString1]));
    assert([signalingSession setDelegate:self]);
    
    /*** GLView ***/
    [remoteView setDelegate:self];
    [remoteView setPAR:1 denominator:1]; // PixelAspectRatio: e.g.  1/1 or 16/9
    [remoteView setFullscreen:NO]; // Set fullscreen value to YES to fill the entire view (will ignore the aspect ratio when resizing)
    const NSInteger selectedSegment = segAnnotations.selectedSegmentIndex;
    SCNSLog(kTAG, @"selectedSegment = %li", (long)selectedSegment);
    switch(selectedSegment) {
        case kSegmentIndexCircle: [remoteView setAnnotationType:SCAnnotationTypeCircle]; break;
        case kSegmentIndexArrow: [remoteView setAnnotationType:SCAnnotationTypeArrow]; break;
        case kSegmentIndexText: [remoteView setAnnotationType:SCAnnotationTypeText]; break;
        case kSegmentIndexFreehand: [remoteView setAnnotationType:SCAnnotationTypeFreehand]; break;
    }
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
    SCNSLog(kTAG, @"???? didReceiveMemoryWarning ????");
}

-(IBAction) onButtonUp:(id)sender {
    if (buttonConnect == sender) {
        if (connecting || connected) {
            if (callSession) {
                [callSession hangup];
            }
            callSession = nil;
            connecting = NO;
            [self showInfo:@"Disconnecting..."];
            assert([signalingSession disConnect]);
        }
        else {
            connecting = YES;
            [buttonConnect setTitle:@"disconnect" forState:UIControlStateNormal];
            assert([signalingSession connect]);
        }
    }
    else if (buttonCall == sender) {
        if (callSession) {
            [callSession hangup];
            callSession = nil;
            [buttonCall setTitle:@"call" forState:UIControlStateNormal];
        }
        else {
            if (pendingOffer) {
                assert(!callSession);
                if (kRejectIncomingCalls) {
                    assert([signalingSession rejectEventCall:pendingOffer]);
                    [buttonCall setTitle:@"call" forState:UIControlStateNormal];
                }
                else {
                    // create a call session from the pending offer
                    assert(callSession = [SCObjcFactory createCallWithSignaling:signalingSession offer:pendingOffer]);
                }
            }
            else {
                // create a call session ex nihilo
                assert(callSession = [SCObjcFactory createCallWithSignaling:signalingSession]);
            }
            if (callSession) {
                assert([callSession setDelegate:self]);
                assert([callSession setVideoDisplays:SCObjcMediaTypeScreenCast local:localView remote:remoteView]);
                assert([callSession setVideoDisplays:SCObjcMediaTypeVideo local:localView remote:remoteView]);
                if (pendingOffer) {
                    assert([callSession acceptEvent:pendingOffer]); // send answer
                }
                else {
                    NSString* remoteID = @"002";
                    [[AppDelegate sharedInstance].config remoteID:&remoteID];
                    assert([callSession call:SCObjcMediaTypeScreenCast destinationID:remoteID]); // send offer
                }
                [buttonCall setTitle:@"hang" forState:UIControlStateNormal];
            }
            pendingOffer = nil;
        }
    }
    else if (buttonFreeze == sender) {
        assert(callSession);
        buttonFreeze.tag = buttonFreeze.tag == 0 ? 1 : 0;
        [buttonFreeze setTitle:(buttonFreeze.tag == 0 ? @"freeze" : @"unfreeze") forState:UIControlStateNormal];
        [remoteView setFreeze:(buttonFreeze.tag == 1)]; // local freeze (pause video rendering)
        [callSession sendFreezeFrame:(buttonFreeze.tag == 1)]; // alert remote party
    }
    else if (buttonClear == sender) {
        [remoteView clearAnnotations];
        if (callSession) {
            [callSession sendClearAnnotations];
        }
    }
    else if(segAnnotations == sender) {
        const NSInteger selectedSegment = segAnnotations.selectedSegmentIndex;
        SCNSLog(kTAG, @"selectSegment(%li)", (long)selectedSegment);
        switch(selectedSegment) {
            case kSegmentIndexCircle: [remoteView setAnnotationType:SCAnnotationTypeCircle]; break;
            case kSegmentIndexArrow: [remoteView setAnnotationType:SCAnnotationTypeArrow]; break;
            case kSegmentIndexText: [remoteView setAnnotationType:SCAnnotationTypeText]; break;
            case kSegmentIndexFreehand: [remoteView setAnnotationType:SCAnnotationTypeFreehand]; break;
        }
    }
}

// SCObjcSignalingDelegate
-(BOOL)signalingDidConnect:(const NSString*)description {
    SCNSLog(kTAG, @"signalingDidConnect(%@)", description);
    connected = YES;
    connecting = NO;
    dispatch_async(dispatch_get_main_queue(), ^{
        [buttonConnect setTitle:@"disconnect" forState:UIControlStateNormal];
        buttonCall.enabled = YES;
        [self showSuccess:@"Connected :)"];
    });
    return YES;
}

// SCObjcSignalingDelegate
-(BOOL)signalingDidDisconnect:(const NSString*)description {
    SCNSLog(kTAG, @"signalingDidDisconnect(%@)", description);
    connected = NO;
    connecting = NO;
    dispatch_async(dispatch_get_main_queue(), ^{
        [buttonConnect setTitle:@"connect" forState:UIControlStateNormal];
        buttonCall.enabled = NO;
        [self showError:@"Disconnected :("];
    });
    callSession = nil;
    return YES;
}

// SCObjcSignalingDelegate
-(BOOL)signalingGotData:(const NSData*)data {
    SCNSLog(kTAG, @"signalingGotData(%@)", [[NSString alloc] initWithData:(NSData*)data encoding:NSUTF8StringEncoding]);
    return YES;
}

// SCObjcSignalingDelegate
-(BOOL)signalingGotEventCall:(id<SCObjcSignalingCallEvent>)e {
    SCNSLog(kTAG, @"signalingGotEventCall(%@)", e);
    //!\Deadlock issue: You must not call any function from 'SCSignaling' class unless you fork a new thread.
    if (callSession) {
        if (![callSession.callID isEqualToString:e.callID]) {
            SCNSLog(kTAG, "Call id mismatch: '%@'<>'%@'", callSession.callID, e.callID);
            return [signalingSession rejectEventCall:e];
        }
        BOOL ret = [callSession acceptEvent:e];
        if ([e.type isEqualToString:kSCOjcSessionCallTypeHangup]) {
            SCNSLog(kTAG, "+++Call ended +++");
            callSession = nil;
            dispatch_async(dispatch_get_main_queue(), ^{
                [self showInfo:@"Call ended"];
                [buttonCall setTitle:@"call" forState:UIControlStateNormal];
            });
        }
        return ret;
    }
    else {
        if ([e.type isEqualToString:kSCOjcSessionCallTypeOffer]) {
            if (callSession || pendingOffer) { // already in call?
                return [signalingSession rejectEventCall:e];
            }
            SCNSLog(kTAG, "+++Incoming call: 'accept'/'reject'? +++");
            pendingOffer = e;
            dispatch_async(dispatch_get_main_queue(), ^{
                [self showInfo:@"Incoming call"];
                [buttonCall setTitle:@"accept" forState:UIControlStateNormal];
            });
        }
        if ([e.type isEqualToString:kSCOjcSessionCallTypeHangup]) {
            if (pendingOffer && [pendingOffer.callID isEqualToString:e.callID]) {
                SCNSLog(kTAG, "+++ pending call cancelled +++");
                pendingOffer = nil;
                dispatch_async(dispatch_get_main_queue(), ^{
                    [self showInfo:@"Pending call cancelled"];
                    [buttonCall setTitle:@"call" forState:UIControlStateNormal];
                });
            }
        }
        
        // Silently ignore any other event type
    }
    return YES;
}

// SCObjcSessionCallDelegate
-(BOOL) callIceStateChanged {
    SCNSLog(kTAG, @"callIceStateChanged(%ld)", (long)callSession.iceState);
    if (callSession.iceState == SCObjcIceStateConnected) {
        return [callSession start];
    }
    return YES;
}

// SCGlviewIOSDelegate
-(void) glviewAnimationStarted {
    SCNSLog(kTAG, @"glviewAnimationStarted(freezed=%d)", remoteView.freezed);
    dispatch_async(dispatch_get_main_queue(), ^{
        buttonFreeze.enabled = YES;
        buttonClear.enabled = YES;
        buttonFreeze.tag = 0;
        [buttonFreeze setTitle:@"freeze" forState:UIControlStateNormal];
    });
}

// SCGlviewIOSDelegate
-(void) glviewAnimationStopped {
    SCNSLog(kTAG, @"glviewAnimationStopped()");
    dispatch_async(dispatch_get_main_queue(), ^{
        buttonFreeze.enabled = NO;
        buttonClear.enabled = NO;
    });
}

// SCGlviewIOSDelegate
-(void) glviewVideoSizeChanged {
    SCNSLog(kTAG, @"glviewVideoSizeChanged(%ix%i)", remoteView.videoWidth, remoteView.videoHeight);
}

// SCGlviewIOSDelegate
-(void) glviewViewportSizeChanged {
    SCNSLog(kTAG, @"glviewViewportSizeChanged(%i,%i,%i,%i)", remoteView.viewportX, remoteView.viewportY, remoteView.viewportWidth, remoteView.viewportHeight);
}

// SCGlviewIOSDelegate
-(void) glviewAnnotationReady:(NSString*)json {
    SCNSLog(kTAG, @"glviewAnnotationReady(%@)", json);
    if (callSession) {
        [callSession sendData:[json dataUsingEncoding:NSUTF8StringEncoding]];
    }
}

-(void)showInfo:(NSString*)msg {
    labelInfo.textColor = [UIColor blackColor];
    labelInfo.text = msg;
}

-(void)showSuccess:(NSString*)msg {
    labelInfo.textColor = [UIColor greenColor];
    labelInfo.text = msg;
}

-(void)showError:(NSString*)msg {
    labelInfo.textColor = [UIColor redColor];
    labelInfo.text = msg;
}

@end
