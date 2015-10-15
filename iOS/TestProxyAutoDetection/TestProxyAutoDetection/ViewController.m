//
//  ViewController.m
//  TestProxyAutoDetection
//
//  Created by Mamadou DIOP on 09/06/15.
//  Copyright (c) 2015 Doubango Telecom. All rights reserved.
//

#import "ViewController.h"

#import <CFNetwork/CFNetwork.h>

#define AUTODETECT_RUNLOOP_MODE         "org.doubango.proxydetect.auto"

static void ProxyAutoConfigurationResultCallback(void *client, CFArrayRef proxyList, CFErrorRef error) {
    CFTypeRef* cfResult = (CFTypeRef*)client;
    if (error != NULL) {
        *cfResult = CFRetain(error);
    } else {
        *cfResult = CFRetain(proxyList);
    }
    CFRunLoopStop(CFRunLoopGetCurrent());
}

@interface ProxyInfo : NSObject
@property NSString* proxyHost;
@property NSString* proxyUserName;
@property NSString* proxyPassword;
@property NSString* proxyType;
@property int proxyPort;
-(BOOL)isValid;
@end

@implementation ProxyInfo
@synthesize proxyHost;
@synthesize proxyUserName;
@synthesize proxyPassword;
@synthesize proxyType;
@synthesize proxyPort;
-(BOOL)isValid {
    return self.proxyType && self.proxyHost && self.proxyPort && ![self.proxyType isEqualToString:(__bridge NSString*)kCFProxyTypeNone];
}
@end

@interface ViewController ()
-(void)ShowAlert:(NSString*)title message:(NSString*)_message;
-(void)findBestProxy:(CFURLRef)cfTargetURL cfProxies:(CFArrayRef)_cfProxies proxyInfo:(ProxyInfo*)_proxyInfo;
@end

@implementation ViewController

@synthesize buttonResolve;
@synthesize textFieldUrl;

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view, typically from a nib.
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

-(IBAction) onButtonUp:(id)sender {
    if (sender == buttonResolve) {
        CFURLRef cfTargetUrl = CFURLCreateWithString(CFAllocatorGetDefault(), (__bridge CFStringRef)textFieldUrl.text, NULL);
        CFDictionaryRef cfProxySettings = NULL;
        CFArrayRef cfProxies = NULL;
        ProxyInfo *proxyInfo = [[ProxyInfo alloc] init];
        if (!cfTargetUrl) {
            [self ShowAlert:@"Error" message:[NSString stringWithFormat:@"Failed to create CFURLRef from %@", textFieldUrl.text]];
            goto resolve_done;
        }
        cfProxySettings = CFNetworkCopySystemProxySettings();
        if (!cfProxySettings) {
            [self ShowAlert:@"Error" message:@"CFNetworkCopySystemProxySettings returned nil"];
            goto resolve_done;
        }
        
        cfProxies = CFNetworkCopyProxiesForURL(cfTargetUrl, cfProxySettings);
        if (!cfProxies) {
            [self ShowAlert:@"Error" message:@"CFNetworkCopyProxiesForURL returned 0-array"];
            goto resolve_done;
        }
        
        [self findBestProxy:cfTargetUrl cfProxies:cfProxies proxyInfo:proxyInfo];
        
        
    resolve_done:
        if (cfTargetUrl) {
            CFRelease(cfTargetUrl);
        }
        if (cfProxySettings) {
            CFRelease(cfProxySettings);
        }
        if (cfProxies) {
            CFRelease(cfProxies);
        }
        NSString* strProxyInfo = [NSString stringWithFormat:@"Type=%@, Host=%@, Port=%i, UserName=%@, Password=%@, isValid=%d",
                              proxyInfo.proxyType, proxyInfo.proxyHost, proxyInfo.proxyPort, proxyInfo.proxyUserName, proxyInfo.proxyPassword, [proxyInfo isValid]];
        NSLog(@"ProxyInfo=%@", strProxyInfo);
        [self ShowAlert:@"Proxy Info" message:strProxyInfo];
    }
}

-(void)ShowAlert:(NSString*)title message:(NSString*)_message {
    UIAlertView *alert = [[UIAlertView alloc] initWithTitle:title
                                                    message:_message
                                                   delegate:nil
                                          cancelButtonTitle:@"OK"
                                          otherButtonTitles:nil];
    [alert show];
}

-(void)findBestProxy:(CFURLRef)cfTargetURL cfProxies:(CFArrayRef)_cfProxies proxyInfo:(ProxyInfo*)_proxyInfo {
    CFDictionaryRef cfProxy;
    CFIndex index = 0;
    CFIndex count = CFArrayGetCount(_cfProxies);
    
    while (index < count && ![_proxyInfo isValid] && (cfProxy = CFArrayGetValueAtIndex(_cfProxies, index++))) {
        CFStringRef cfProxyType = CFDictionaryGetValue(cfProxy, (const void*)kCFProxyTypeKey);
        if (!cfProxyType) {
            continue;
        }
        NSLog(@"Found at %li proxy type = %@", (index - 1), (__bridge NSString*)cfProxyType);
        if (CFEqual(cfProxyType, kCFProxyTypeNone)) {
            continue;
        }
        else if (CFEqual(cfProxyType, kCFProxyTypeHTTP) || CFEqual(cfProxyType, kCFProxyTypeHTTPS) || CFEqual(cfProxyType, kCFProxyTypeSOCKS)) {
            // "kCFProxyTypeHTTPS" means the url is "https://" not the connection should be TLS
            CFStringRef cfHostName = (CFStringRef)CFDictionaryGetValue(cfProxy, (const void*)kCFProxyHostNameKey);
            if (cfHostName) {
                CFNumberRef cfPortNumber = (CFNumberRef)CFDictionaryGetValue(cfProxy, (const void*)kCFProxyPortNumberKey);
                if (cfPortNumber) {
                    int port = 0;
                    if (!CFNumberGetValue(cfPortNumber, kCFNumberSInt32Type, &port)) {
                        _proxyInfo.proxyPort = 0;
                        continue;
                    }
                    _proxyInfo.proxyPort = port;
                    _proxyInfo.proxyHost = CFBridgingRelease(cfHostName);
                    _proxyInfo.proxyType = CFBridgingRelease(cfProxyType);
                    CFStringRef cfStringName = (CFStringRef)CFDictionaryGetValue(cfProxy, (const void*)kCFProxyUsernameKey);
                    if (cfStringName) {
                        _proxyInfo.proxyUserName = CFBridgingRelease(cfStringName);
                    }
                    CFStringRef cfPassword = (CFStringRef)CFDictionaryGetValue(cfProxy, (const void*)kCFProxyPasswordKey);
                    if (cfPassword) {
                        _proxyInfo.proxyPassword = CFBridgingRelease(cfPassword);
                    }
                }
            }
        }
        else if (CFEqual(cfProxyType, kCFProxyTypeAutoConfigurationURL)) {
            CFURLRef cfPACUrl = (CFURLRef)CFDictionaryGetValue(cfProxy, (const void*)kCFProxyAutoConfigurationURLKey);
            if (cfPACUrl) {
                NSLog(@"Found at %li PACUrl = %@", (index - 1), (__bridge NSString*)CFURLGetString(cfPACUrl));
                CFTypeRef cfResult = NULL;
                CFStreamClientContext context = { 0, &cfResult, NULL, NULL, NULL };
                CFRunLoopSourceRef cfrunLoop = CFNetworkExecuteProxyAutoConfigurationURL(cfPACUrl,
                                                                                         cfTargetURL,
                                                                                         ProxyAutoConfigurationResultCallback,
                                                                                         &context);
                if (!cfrunLoop) {
                    NSLog(@"CFNetworkExecuteProxyAutoConfigurationURL(%li, %@) failed", (index - 1), cfPACUrl);
                    continue;
                }
                static const CFStringRef kPrivateRunloopMode = CFSTR(AUTODETECT_RUNLOOP_MODE);
                CFRunLoopAddSource(CFRunLoopGetCurrent(), cfrunLoop, kPrivateRunloopMode);
                CFRunLoopRunInMode(kPrivateRunloopMode, DBL_MAX, false);
                CFRunLoopRemoveSource(CFRunLoopGetCurrent(), cfrunLoop, kPrivateRunloopMode);
                if (cfResult == NULL) {
                    NSLog(@"Result from ProxyAutoConfigurationResultCallback is nil");
                    continue;
                }
                if (CFGetTypeID(cfResult) == CFErrorGetTypeID()) {
                    CFStringRef cfErrorDescription = CFErrorCopyDescription ((CFErrorRef)cfResult);
                    NSLog(@"Result from ProxyAutoConfigurationResultCallback is error: %@", cfErrorDescription);
                    CFRelease(cfErrorDescription);
                }
                else if (CFGetTypeID(cfResult) == CFArrayGetTypeID()) {
                    NSLog(@"Result from ProxyAutoConfigurationResultCallback is array");
                    [self findBestProxy:cfTargetURL cfProxies:(CFArrayRef)cfResult proxyInfo:_proxyInfo];
                }
                CFRelease(cfResult);
            }
            else {
                NSLog(@"PACUrl at %li is nil", (index - 1));
            }
        }
    }
}

@end
