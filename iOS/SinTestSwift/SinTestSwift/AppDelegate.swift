//
//  AppDelegate.swift
//  SinTestSwift
//
//  Created by Mamadou DIOP on 25/04/15.
//  Copyright (c) 2015 Doubango Telecom. All rights reserved.
//

import UIKit

@UIApplicationMain
class AppDelegate: UIResponder, UIApplicationDelegate {
    
    var window: UIWindow?
    var config: SCObjcConfig?
    var viewController: ViewController?
    
    
    func application(application: UIApplication, didFinishLaunchingWithOptions launchOptions: [NSObject: AnyObject]?) -> Bool {
        // Override point for customization after application launch.
        
        let configPath:String? = NSBundle.mainBundle().pathForResource("config", ofType:"json")
        if (NSFileManager.defaultManager().fileExistsAtPath(configPath!)) {
            NSLog("%[SinTestSwift::AppDelegate] Found config file at: %@\n", configPath!)
            config = SCObjcFactory.createConfigWithFile(configPath!)
            assert(config != nil)
        }
        else {
            NSLog("%[SinTestSwift::AppDelegate] Failed to find config file at: %@\n", configPath!)
            assert(false)
        }
        
        if (application.respondsToSelector("registerUserNotificationSettings:")) {
            let notificationType = UIUserNotificationType.Alert.union(UIUserNotificationType.Badge).union(UIUserNotificationType.Sound)
            let settings = UIUserNotificationSettings(forTypes: notificationType, categories: nil)
            application.registerUserNotificationSettings(settings)
        }
        else {
            let notificationType = UIRemoteNotificationType.Alert.union(UIRemoteNotificationType.Badge).union(UIRemoteNotificationType.Sound)
            application.registerForRemoteNotificationTypes(notificationType)
        }
        
        viewController = self.window!.rootViewController as? ViewController
        
        return true
    }
    
    func applicationWillResignActive(application: UIApplication) {
        // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
        // Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
        NSLog("applicationWillResignActive")
    }
    
    func applicationDidEnterBackground(application: UIApplication) {
        // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
        // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
        NSLog("applicationDidEnterBackground")
    }
    
    func applicationWillEnterForeground(application: UIApplication) {
        // Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
        NSLog("applicationWillEnterForeground")
    }
    
    func applicationDidBecomeActive(application: UIApplication) {
        // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
        NSLog("applicationDidBecomeActive")
    }
    
    func applicationWillTerminate(application: UIApplication) {
        // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
        NSLog("applicationWillTerminate")
        
        if (viewController != nil) {
            viewController?.applicationWillTerminate()
        }
    }
    
    func application(application: UIApplication, didReceiveLocalNotification notification: UILocalNotification) {
        NSLog("didReceiveLocalNotification")
        let type:String = notification.userInfo!["type"] as! String
        if (type == "offer") {
            let callID:String? = notification.userInfo!["callID"] as? String
            if (callID != nil) {
                // use "callID" to initialize your view, accept/reject incoming call
            }
        }
        else if (type == "chat") {
            //let username:String = notification.userInfo!["username"] as! String
            //let message:String = notification.userInfo!["message"] as! String
            // use "username" and "message" to fill your chat window
        }
        application.applicationIconBadgeNumber -= notification.applicationIconBadgeNumber
    }
}


