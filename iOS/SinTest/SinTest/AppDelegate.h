//
//  AppDelegate.h
//  SinTest
//
//  Created by Mamadou DIOP on 22/04/15.
//  Copyright (c) 2015 Doubango Telecom. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "sincity/sc_api_objc.h"

@class ViewController;

@interface AppDelegate : UIResponder <UIApplicationDelegate> {
    ViewController* viewController;
}
+(AppDelegate*)sharedInstance;

@property (strong, nonatomic) UIWindow *window;
@property (readonly) NSObject<SCObjcConfig>* config;

@end

