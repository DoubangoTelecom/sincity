//
//  AppDelegate.m
//  SinTest
//
//  Created by Mamadou DIOP on 22/04/15.
//  Copyright (c) 2015 Doubango Telecom. All rights reserved.
//

#import "AppDelegate.h"
#import "ViewController.h"

#define kTAG @"[SinTest::AppDelegate]"

@interface AppDelegate ()

@end

@implementation AppDelegate

@synthesize config;

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    // Override point for customization after application launch.
    
    NSString* configPath = [[NSBundle mainBundle] pathForResource:@"config" ofType:@"json"];
    if ([[NSFileManager defaultManager] fileExistsAtPath:configPath]) {
        SCNSLog(kTAG, @"Found config file at: %@", configPath);
        assert(config = [SCObjcFactory createConfigWithFile:configPath]);
    }
    else {
        SCNSLog(kTAG, @"Failed to find config file at: %@", configPath);
        assert(false);
    }
    
    if ([application respondsToSelector:@selector(registerUserNotificationSettings:)]) {
        [application registerUserNotificationSettings:[UIUserNotificationSettings settingsForTypes:(UIUserNotificationTypeSound | UIUserNotificationTypeBadge | UIUserNotificationTypeAlert) categories:nil]];
    } else {
        [application registerForRemoteNotificationTypes:(UIRemoteNotificationTypeBadge | UIRemoteNotificationTypeSound | UIRemoteNotificationTypeAlert)];
    }
    
    viewController = (ViewController*) self.window.rootViewController;
    
    return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application {
    // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
    // Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
    SCNSLog(kTAG, @"applicationWillResignActive");
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
    // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
    // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
    SCNSLog(kTAG, @"applicationDidEnterBackground");
#if __IPHONE_OS_VERSION_MIN_REQUIRED >= 40000
    [application setKeepAliveTimeout:600 handler: ^{
        SCNSLog(kTAG, @"applicationDidEnterBackground:: setKeepAliveTimeout:handler^");
    }];
#endif
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
    // Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
    SCNSLog(kTAG, @"applicationWillEnterForeground");
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
    // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
    SCNSLog(kTAG, @"applicationDidBecomeActive");
}

- (void)applicationWillTerminate:(UIApplication *)application {
    // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
    
    SCNSLog(kTAG, @"applicationWillTerminate");
    if (viewController) {
        [viewController applicationWillTerminate];
        viewController = nil;
    }
    
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)application:(UIApplication *)application didReceiveLocalNotification:(UILocalNotification *)notification{
    SCNSLog(kTAG, @"didReceiveLocalNotification");
    NSString *notifType = [notification.userInfo objectForKey:@"type"];
    if ([notifType isEqualToString:@"offer"]) {
        NSString *callID = [notification.userInfo objectForKey:@"callID"];
        if (callID) {
            // use "callID" to initialize your view, accept/reject incoming call
        }
    }
    else if ([notifType isEqualToString:@"chat"]) {
        NSString* username = [notification.userInfo objectForKey:@"username"];
        NSString* message = [notification.userInfo objectForKey:@"message"];
        if (username && message) {
            // use "username" and "message" to fill your chat window
        }
    }
    application.applicationIconBadgeNumber -= notification.applicationIconBadgeNumber;
}

+(AppDelegate*) sharedInstance {
    return ((AppDelegate*) [[UIApplication sharedApplication] delegate]);
}

@end
