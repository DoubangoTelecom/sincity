//
//  ViewController.swift
//  SinTestSwift
//
//  Created by Mamadou DIOP on 25/04/15.
//  Copyright (c) 2015 Doubango Telecom. All rights reserved.
//

// https://developer.apple.com/library/ios/documentation/Swift/Conceptual/BuildingCocoaApps/InteractingWithCAPIs.html

import UIKit

let kTAG:String = "[SinTestSwift::ViewController]"
let kRejectIncomingCalls:Bool = false

let kSegmentIndexCircle:Int = 0
let kSegmentIndexArrow:Int = 1
let kSegmentIndexText:Int = 2
let kSegmentIndexFreehand:Int = 3

let kDefaultMediaType:SCObjcMediaType = (SCObjcMediaType.ScreenCast)
//let kDefaultMediaType:SCObjcMediaType = (SCObjcMediaType.Video)
//let kDefaultMediaType:SCObjcMediaType = SCObjcMediaType(rawValue: (SCObjcMediaType.ScreenCast.rawValue | SCObjcMediaType.Audio.rawValue))!
//let kDefaultMediaType:SCObjcMediaType = SCObjcMediaType(rawValue: (SCObjcMediaType.Video.rawValue | SCObjcMediaType.Audio.rawValue))!

class ViewController: UIViewController, SCObjcSignalingDelegate, SCObjcSessionCallDelegate, SCGlviewIOSDelegate {
    @IBOutlet weak var buttonConnect:UIButton?
    @IBOutlet weak var buttonCall:UIButton?
    @IBOutlet weak var buttonFreeze:UIButton?
    @IBOutlet weak var buttonClear:UIButton?
    @IBOutlet weak var labelInfo:UILabel?
    @IBOutlet weak var localView:UIView?
    @IBOutlet weak var segAnnotations:UISegmentedControl?
    @IBOutlet weak var remoteView:SCGlviewIOS?
    var signalingSession:SCObjcSignaling? = nil
    var callSession:SCObjcSessionCall? = nil
    var pendingOffer:SCObjcSignalingCallEvent? = nil
    var connecting:Bool? = false
    var connected:Bool? = false
    
    override func viewDidLoad() {
        super.viewDidLoad()
        // Do any additional setup after loading the view, typically from a nib.
        buttonFreeze!.tag = 0
        buttonFreeze!.enabled = false
        buttonClear!.enabled = false
        buttonCall!.enabled = false
        
        let appDelegate:AppDelegate! = UIApplication.sharedApplication().delegate as! AppDelegate
        
        var nsString1:NSString? = nil, nsString2:NSString? = nil, nsString3:NSString? = nil, nsString4:NSString? = nil
        var ui1:UInt = 0, ui2:UInt = 0
        var i1:Int32 = 0, i2:Int32 = 0
        var b1:ObjCBool = false
        
        /*** Engine ***/
        // debug level
        var debugLevel:SCObjcDebugLevel? = SCObjcDebugLevelInfo
        if (appDelegate.config!.debugLevel(&debugLevel!)) {
            assert(SCObjcEngine.setDebugLevel(debugLevel!))
        }
        // init
        assert(appDelegate.config!.localID(&nsString1))
        assert(SCObjcEngine.initWithUserId(nsString1 as! String))
        // ssl certificates
        assert(appDelegate.config!.sslCertificates(&nsString1, privateKey:&nsString2, CA:&nsString3))
        assert(SCObjcEngine.setSSLCertificates(NSBundle.mainBundle().pathForResource(nsString1?.lastPathComponent.stringByDeletingPathExtension, ofType:nsString1?.pathExtension),
            privateKey: NSBundle.mainBundle().pathForResource(nsString2?.lastPathComponent.stringByDeletingPathExtension, ofType:nsString2?.pathExtension),
            CA:NSBundle.mainBundle().pathForResource(nsString3?.lastPathComponent.stringByDeletingPathExtension, ofType:nsString3?.pathExtension)))
        
        // video pref size
        if (appDelegate.config!.videoPrefSize(&nsString1)) {
            assert(SCObjcEngine.setVideoPrefSize(nsString1 as! String))
        }
        // video fps
        if (appDelegate.config!.videoFps(&i1)) {
            assert(SCObjcEngine.setVideoFps(i1))
        }
        // video bandwidth up max
        if (appDelegate.config!.videoBandwidthUpMax(&i1)) {
            assert(SCObjcEngine.setVideoBandwidthUpMax(i1))
        }
        // video bandwidth down max
        if (appDelegate.config!.videoBandwidthDownMax(&i1)) {
            assert(SCObjcEngine.setVideoBandwidthDownMax(i1))
        }
        // video motion rank
        if (appDelegate.config!.videoMotionRank(&i1)) {
            assert(SCObjcEngine.setVideoMotionRank(i1))
        }
        // video congestion control enabled
        if (appDelegate.config!.videoCongestionCtrlEnabled(&b1)) {
            assert(SCObjcEngine.setVideoCongestionCtrlEnabled(b1.boolValue))
        }
        // video jitter buffer enabled
        if (appDelegate.config!.videoJbEnabled(&b1)) {
            assert(SCObjcEngine.setVideoJbEnabled(b1.boolValue))
        }
        // video zero artifacts enabled
        if (appDelegate.config!.videoZeroArtifactsEnabled(&b1)) {
            assert(SCObjcEngine.setVideoZeroArtifactsEnabled(b1.boolValue))
        }
        // video AVPF tail
        if (appDelegate.config!.videoAvpfTail(&i1, max:&i2)) {
            assert(SCObjcEngine.setVideoAvpfTail(i1, max:i2))
        }
        #if !TARGET_OS_IPHONE // Do not enable Doubango AEC. Native AEC always ON on iOS :)
            // audio echo supp enabled
            if (appDelegate.config!.audioEchoSuppEnabled(&b1)) {
                assert(SCObjcEngine.setAudioEchoSuppEnabled(b1.boolValue));
            }
            // audio echo tail
            if (appDelegate.config!.audioEchoTail(&i1)) {
                assert(SCObjcEngine.setAudioEchoTail(i1))
            }
        #endif
        // natt ice servers
        if (appDelegate.config!.nattIceServersCount(&ui1)) {
            var iceServer:SCObjcIceServer?;
            for (ui2 = 0; ui2 < ui1; ++ui2) {
                assert(appDelegate.config!.nattIceServersAt(ui2, server:&iceServer));
                assert(SCObjcEngine.addNattIceServer(iceServer!.`protocol`, serverHost:iceServer!.host, serverPort:iceServer!.port, useTurn:iceServer!.turnEnabled, useStun:iceServer!.stunEnabled, userName:iceServer!.login, password:iceServer!.password))
            }
        }
        // natt ice-stun enabled
        if (appDelegate.config!.nattIceStunEnabled(&b1)) {
            assert(SCObjcEngine.setNattIceStunEnabled(b1.boolValue))
        }
        // natt ice-turn enabled
        if (appDelegate.config!.nattIceTurnEnabled(&b1)) {
            assert(SCObjcEngine.setNattIceTurnEnabled(b1.boolValue))
        }
        // webproxy
        var port:UInt16 = 0
        if (appDelegate.config!.webproxyAutoDetect(&b1)) {
            assert(SCObjcEngine.setWebProxyAutodetect(b1.boolValue));
        }
        if (appDelegate.config!.webproxyInfo(&nsString1, host:&nsString2, port:&port, login:&nsString3, password:&nsString4)) {
            assert(SCObjcEngine.setWebProxyInfo(nsString1 as! String, host:nsString2 as! String, port:port, login:nsString3 as! String, password:nsString4 as! String));
        }
        
        /*** signaling ***/
        assert(appDelegate.config!.connectionUrl(&nsString1));
        signalingSession = SCObjcFactory.createSignalingWithConnectionUrl(nsString1 as! String)
        assert(signalingSession != nil)
        assert(signalingSession!.setDelegate(self))
        /*** GLView ***/
        remoteView!.setDelegate(self)
        remoteView!.setPAR(1, denominator:1) // PixelAspectRatio: e.g.  1/1 or 16/9
        remoteView!.setFullscreen(false) // Set fullscreen value to YES to fill the entire view (will ignore the aspect ratio when resizing)
        let selectedSegment:NSInteger = segAnnotations!.selectedSegmentIndex
        NSLog("%@ selectedSegment = %li", kTAG, selectedSegment)
        switch(selectedSegment) {
        case kSegmentIndexCircle: remoteView!.setAnnotationType(SCAnnotationTypeCircle); break
        case kSegmentIndexArrow: remoteView!.setAnnotationType(SCAnnotationTypeArrow); break
        case kSegmentIndexText: remoteView!.setAnnotationType(SCAnnotationTypeText); break
        case kSegmentIndexFreehand: remoteView!.setAnnotationType(SCAnnotationTypeFreehand); break
        default: break;
        }
    }
    
    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
        NSLog("%@ ???? didReceiveMemoryWarning ????", kTAG)
    }
    
    @IBAction func onButtonUp(sender: UIControl) {
        if (buttonConnect == sender) {
            if (connecting! || connected!) {
                if (callSession != nil) {
                    callSession!.hangup()
                }
                callSession = nil
                connecting = false
                self.showInfo("Disconnecting...")
                assert(signalingSession!.disConnect())
            }
            else {
                connecting = true
                buttonConnect!.setTitle("disconnect", forState:UIControlState.Normal)
                assert(signalingSession!.connect())
            }
        }
        else if (buttonCall == sender) {
            if (callSession != nil) {
                callSession!.hangup()
                callSession = nil
                buttonCall!.setTitle("call", forState:UIControlState.Normal)
            }
            else {
                if (pendingOffer != nil) {
                    assert(callSession == nil)
                    if (kRejectIncomingCalls) {
                        assert(signalingSession!.rejectEventCall(pendingOffer))
                        buttonCall!.setTitle("call", forState:UIControlState.Normal)
                    }
                    else {
                        // create a call session from the pending offer
                        callSession = SCObjcFactory.createCallWithSignaling(signalingSession, offer:pendingOffer)
                        assert(callSession != nil)
                    }
                }
                else {
                    // create a call session ex nihilo
                    callSession = SCObjcFactory.createCallWithSignaling(signalingSession)
                    assert(callSession != nil)
                }
                if (callSession != nil) {
                    assert(callSession!.setDelegate(self))
                    assert(callSession!.setVideoDisplays(.ScreenCast, local:localView, remote:remoteView))
                    assert(callSession!.setVideoDisplays(.Video, local:localView, remote:remoteView))
                    if (pendingOffer != nil) {
                        assert(callSession!.acceptEvent(pendingOffer)) // send answer
                    }
                    else {
                        var remoteID:NSString? = "002"
                        let appDelegate:AppDelegate! = UIApplication.sharedApplication().delegate as! AppDelegate
                        appDelegate.config?.remoteID(&remoteID)
                        assert(callSession!.call(kDefaultMediaType, destinationID:(remoteID as! String))) // send offer
                    }
                    buttonCall!.setTitle("hang", forState:UIControlState.Normal)
                }
                pendingOffer = nil
            }
        }
        else if (buttonFreeze == sender) {
            assert(callSession != nil)
            buttonFreeze!.tag = buttonFreeze!.tag == 0 ? 1 : 0
            buttonFreeze!.setTitle((buttonFreeze!.tag == 0 ? "freeze" : "unfreeze"), forState:UIControlState.Normal)
            remoteView!.setFreeze(buttonFreeze!.tag == 1) // local freeze (pause video rendering)
            callSession!.sendFreezeFrame(buttonFreeze!.tag == 1) // alert remote party
        }
        else if (buttonClear == sender) {
            remoteView!.clearAnnotations()
            if (callSession != nil) {
                callSession!.sendClearAnnotations()
            }
        }
        else if(segAnnotations == sender) {
            let selectedSegment:NSInteger = segAnnotations!.selectedSegmentIndex
            NSLog("%@ selectSegment(%li)", kTAG, selectedSegment);
            switch(selectedSegment) {
            case kSegmentIndexCircle: remoteView!.setAnnotationType(SCAnnotationTypeCircle); break
            case kSegmentIndexArrow: remoteView!.setAnnotationType(SCAnnotationTypeArrow); break
            case kSegmentIndexText: remoteView!.setAnnotationType(SCAnnotationTypeText); break
            case kSegmentIndexFreehand: remoteView!.setAnnotationType(SCAnnotationTypeFreehand); break
            default: break
            }
        }
    }
    
    func applicationWillTerminate() -> Void {
        assert(signalingSession!.setDelegate(nil))
        remoteView!.setDelegate(nil)
        if (connecting! || connected!) {
            if (callSession != nil) {
                callSession!.hangup()
            }
            callSession = nil
            connecting = false
            assert(signalingSession!.disConnect())
        }
    }
    
    // SCObjcSignalingDelegate
    func signalingDidConnect(description: String) -> Bool {
        NSLog("%@ signalingDidConnect(%@)", kTAG, description)
        connected = true
        connecting = false
        dispatch_async(dispatch_get_main_queue()) {
            self.buttonConnect!.setTitle("disconnect", forState:UIControlState.Normal)
            self.buttonCall!.enabled = true
            self.showSuccess("Connected :)")
        }
        return true
    }
    
    // SCObjcSignalingDelegate
    func signalingDidDisconnect(description: String) -> Bool {
        NSLog("%@ signalingDidDisconnect(%@)", kTAG, description)
        connected = false
        connecting = false
        dispatch_async(dispatch_get_main_queue()) {
            self.buttonConnect!.setTitle("connect", forState:UIControlState.Normal)
            self.buttonCall!.enabled = false
            self.showError("Disconnected :(")
        }
        callSession = nil
        return true
    }
    
    // SCObjcSignalingDelegate
    func signalingGotData(data: NSData) -> Bool {
        NSLog("%@ signalingGotData(%@)", kTAG, NSString(data:data, encoding:NSUTF8StringEncoding)!)
        return true
    }
    
    // SCObjcSignalingDelegate
    func signalingGotEventCall(e: SCObjcSignalingCallEvent) -> Bool {
        NSLog("%@ signalingGotEventCall(%@)", kTAG, e.description)
        self.notifyCallEvent(e)
        //!\Deadlock issue: You must not call any function from 'SCSignaling' class unless you fork a new thread.
        if (callSession != nil) {
            if (callSession!.callID != e.callID) {
                NSLog("%@ Call id mismatch: '%@'<>'%@'", kTAG, callSession!.callID, e.callID)
                return signalingSession!.rejectEventCall(e)
            }
            let ret:Bool = callSession!.acceptEvent(e)
            if (e.type == kSCOjcSessionCallTypeHangup) {
                NSLog("%@ +++Call ended +++", kTAG)
                callSession = nil
                dispatch_async(dispatch_get_main_queue()) {
                    self.showInfo("Call ended")
                    self.buttonCall!.setTitle("call", forState:UIControlState.Normal)
                }
            }
            return ret
        }
        else {
            if (e.type == kSCOjcSessionCallTypeOffer) {
                if (callSession != nil || pendingOffer != nil) { // already in call?
                    return signalingSession!.rejectEventCall(e)
                }
                NSLog("%@ +++Incoming call: 'accept'/'reject'? +++", kTAG)
                pendingOffer = e
                dispatch_async(dispatch_get_main_queue()) {
                    self.showInfo("Incoming call")
                    self.buttonCall!.setTitle("accept", forState:UIControlState.Normal)
                }
            }
            if (e.type == kSCOjcSessionCallTypeHangup) {
                if (pendingOffer != nil && pendingOffer!.callID == e.callID) {
                    NSLog("%@ +++ pending call cancelled +++", kTAG)
                    pendingOffer = nil
                    dispatch_async(dispatch_get_main_queue()) {
                        self.showInfo("Pending call cancelled")
                        self.buttonCall!.setTitle("call", forState:UIControlState.Normal)
                    }
                }
            }
            
            // Silently ignore any other event type
        }
        return true
    }
    
    // SCObjcSignalingDelegate
    func signalingGotChatMessage(message: String, username: String) -> Bool {
        NSLog("%@ signalingGotChatMessage(%@, %@)", kTAG, message, username);
        // show notifications only when app is in background
        if (UIApplication.sharedApplication().applicationState == .Background) {
            let localNotif:UILocalNotification!  = UILocalNotification()
            if (localNotif != nil) {
                let stringAlert:String = String(format:"%@: %@", username, message)
                localNotif.alertBody = stringAlert
                localNotif.soundName = UILocalNotificationDefaultSoundName
                localNotif.applicationIconBadgeNumber = ++UIApplication.sharedApplication().applicationIconBadgeNumber
                localNotif.userInfo = [
                    "type": "chat",
                    "message": message,
                    "username": username
                ]
                UIApplication.sharedApplication().presentLocalNotificationNow(localNotif)
            }
        }
        return true
    }
    
    // SCObjcSessionCallDelegate
    func callIceStateChanged() -> Bool {
        NSLog("%@ callIceStateChanged(%i)", kTAG, callSession!.iceState.rawValue)
        if (callSession!.iceState == .Connected) {
            return callSession!.start()
        }
        return true
    }
    
    // SCObjcSessionCallDelegate
    func callInterruptionChanged(interrupted: Bool) -> Bool {
        NSLog("%@ callIceStateChanged(%@)", kTAG, interrupted ? "YES" : "NO")
        // For example, you can send a JSON message to alert the remote party that the session is interrupted because of incoming GSM call
        // You dont need to call any AudioUnit function (up to Doubango native code)
        return true;
    }
    
    // SCGlviewIOSDelegate
    func glviewAnimationStarted() {
        NSLog("%@ glviewAnimationStarted(freezed=%@)", kTAG, remoteView!.freezed ? "YES" : "NO");
        dispatch_async(dispatch_get_main_queue()) {
            self.buttonFreeze!.enabled = true
            self.buttonClear!.enabled = true
            self.buttonFreeze!.tag = 0
            self.buttonFreeze!.setTitle("freeze", forState:UIControlState.Normal)
        }
    }
    
    // SCGlviewIOSDelegate
    func glviewAnimationStopped() {
        NSLog("%@ glviewAnimationStopped()", kTAG)
        dispatch_async(dispatch_get_main_queue()){
            self.buttonFreeze!.enabled = false
            self.buttonClear!.enabled = false
        }
    }
    
    // SCGlviewIOSDelegate
    func glviewVideoSizeChanged() {
        NSLog("%@ glviewVideoSizeChanged(%ix%i)", kTAG, remoteView!.videoWidth, remoteView!.videoHeight)
    }
    
    // SCGlviewIOSDelegate
    func glviewViewportSizeChanged() {
        NSLog("%@ glviewViewportSizeChanged(%i,%i,%i,%i)", kTAG, remoteView!.viewportX, remoteView!.viewportY, remoteView!.viewportWidth, remoteView!.viewportHeight)
    }
    
    // SCGlviewIOSDelegate
    func glviewAnnotationReady(json: String) {
        NSLog("%@ glviewAnnotationReady(%@)", kTAG, json)
        if (callSession != nil) {
            callSession!.sendData(json.dataUsingEncoding(NSUTF8StringEncoding))
        }
    }
    
    func showInfo(msg: String) {
        labelInfo!.textColor = UIColor.blackColor()
        labelInfo!.text = msg
    }
    
    func showSuccess(msg: String) {
        labelInfo!.textColor = UIColor.greenColor()
        labelInfo!.text = msg
    }
    
    func showError(msg: String) {
        labelInfo!.textColor = UIColor.redColor()
        labelInfo!.text = msg
    }
    
    func notifyCallEvent(e:SCObjcSignalingCallEvent) {
        // show notifications only when app is in background
        if (UIApplication.sharedApplication().applicationState == .Background) {
            // show notification for incoming calls only
            if (e.type == kSCOjcSessionCallTypeOffer) {
                let localNotif:UILocalNotification!  = UILocalNotification()
                if (localNotif != nil) {
                    let stringAlert:String = String(format:"Call from \n %@", e.from)
                    localNotif.alertBody = stringAlert
                    localNotif.soundName = UILocalNotificationDefaultSoundName
                    localNotif.applicationIconBadgeNumber = ++UIApplication.sharedApplication().applicationIconBadgeNumber
                    localNotif.userInfo = [
                        "type": e.type,
                        "from": e.from,
                        "to": e.to,
                        "callID": e.callID,
                        "transactionID": e.transactionID,
                        "sdp": e.sdp
                    ]
                    UIApplication.sharedApplication().presentLocalNotificationNow(localNotif)
                }
            }
        }
    }
}

