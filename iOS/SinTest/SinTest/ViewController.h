//
//  ViewController.h
//  SinTest
//
//  Created by Mamadou DIOP on 22/04/15.
//  Copyright (c) 2015 Doubango Telecom. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "sincity/sc_api_objc.h"
#import "sincity/sc_glview_ios.h"

@interface ViewController : UIViewController<SCObjcSignalingDelegate, SCObjcSessionCallDelegate, SCGlviewIOSDelegate> {
    IBOutlet UIButton *buttonConnect;
    IBOutlet UIButton *buttonCall;
    IBOutlet UIButton *buttonFreeze;
    IBOutlet UIButton *buttonClear;
    IBOutlet UILabel *labelInfo;
    IBOutlet UIView* localView;
    IBOutlet UISegmentedControl* segAnnotations;
    IBOutlet SCGlviewIOS* remoteView;
    NSObject<SCObjcSignaling>* signalingSession;
    NSObject<SCObjcSessionCall>* callSession;
    NSMutableDictionary* pendingOffers;
    
    BOOL connecting;
    BOOL connected;
}
-(IBAction) onButtonUp:(id)sender;
-(void) applicationWillTerminate;
-(BOOL) acceptPendingOffer:(NSString*)callID;
-(BOOL) rejectPendingOffer:(NSString*)callID;

@property (retain, nonatomic) IBOutlet UIButton *buttonConnect;
@property (retain, nonatomic) IBOutlet UIButton *buttonCall;
@property (retain, nonatomic) IBOutlet UIButton *buttonFreeze;
@property (retain, nonatomic) IBOutlet UIButton *buttonClear;
@property (retain, nonatomic) IBOutlet UILabel *labelInfo;
@property (retain, nonatomic) IBOutlet UIView* localView;
@property (retain, nonatomic) IBOutlet UISegmentedControl* segAnnotations;
@property (retain, nonatomic) IBOutlet SCGlviewIOS* remoteView;
@end

