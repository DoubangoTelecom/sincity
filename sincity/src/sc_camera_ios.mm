#import "sincity/sc_camera_ios.h"
#import "sincity/sc_debug.h"
#import "sincity/sc_common.h"

#if SC_UNDER_IPHONE

//
//	Private
//
@interface SCCameraIOS (Private)

#if SC_PRODUCER_IOS_HAVE_VIDEO_CAPTURE
+ (AVCaptureDevice *)cameraAtPosition:(AVCaptureDevicePosition)position;
#endif /* SC_PRODUCER_IOS_HAVE_VIDEO_CAPTURE */

@end /* SC_PRODUCER_IOS_HAVE_VIDEO_CAPTURE */

@implementation SCCameraIOS (Private)

#if SC_PRODUCER_IOS_HAVE_VIDEO_CAPTURE

+ (AVCaptureDevice *)cameraAtPosition:(AVCaptureDevicePosition)position{
    NSArray *cameras = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
    for (AVCaptureDevice *device in cameras){
        if (device.position == position){
            return device;
        }
    }
    return [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
}

#endif /* SC_PRODUCER_IOS_HAVE_VIDEO_CAPTURE */

@end


//
//	Default implementation
//
@implementation SCCameraIOS

#if SC_PRODUCER_IOS_HAVE_VIDEO_CAPTURE

+ (AVCaptureDevice *)frontFacingCamera{
    return [SCCameraIOS cameraAtPosition:AVCaptureDevicePositionFront];
}

+ (AVCaptureDevice *)backCamera{
    return [SCCameraIOS cameraAtPosition:AVCaptureDevicePositionBack];
}

#endif /* SC_PRODUCER_IOS_HAVE_VIDEO_CAPTURE */

+ (BOOL) setPreview: (UIView*)preview{
#if SC_PRODUCER_IOS_HAVE_VIDEO_CAPTURE
    static UIView* sPreview = nil;
    static AVCaptureSession* sCaptureSession = nil;
    
    if(preview == nil){
        // stop preview
        if(sCaptureSession && [sCaptureSession isRunning]){
            [sCaptureSession stopRunning];
        }
        // remove all sublayers
        if(sPreview){
            for(CALayer *ly in sPreview.layer.sublayers){
                if([ly isKindOfClass: [AVCaptureVideoPreviewLayer class]]){
                    [ly removeFromSuperlayer];
                    break;
                }
            }
        }
        return YES;
    }
    
    if (!sCaptureSession) {
        NSError *error = nil;
        AVCaptureDeviceInput *videoInput = [AVCaptureDeviceInput deviceInputWithDevice: [SCCameraIOS frontFacingCamera] error:&error];
        if (!videoInput){
            SC_DEBUG_INFO_EX(kSCMobuleNameiOSCamera, "Failed to get video input: %s", (error && error.description) ? [error.description UTF8String] : "unknown");
            return NO;
        }
        
        sCaptureSession = [[AVCaptureSession alloc] init];
        [sCaptureSession addInput:videoInput];
    }
    
    // start capture if not already done or view did changed
    if (sPreview != preview || ![sCaptureSession isRunning]) {
        [sPreview release];
        sPreview = [preview retain];
        
        /*
        if (([previewLayer respondsToSelector:@selector(connection)] ? [previewLayer connection].videoOrientation : previewLayer.orientationSupported)) {
            // WARNING: -[<AVCaptureVideoPreviewLayer: 0x70235f60> setOrientation:] is deprecated.  Please use AVCaptureConnection's -setVideoOrientation:
            if ([previewLayer respondsToSelector:@selector(connection)]) {
                [previewLayer.connection setVideoOrientation:mOrientation];
            } else {
                previewLayer.orientation    = mOrientation;
            }
        }
        */
        
        
        AVCaptureVideoPreviewLayer* previewLayer = [AVCaptureVideoPreviewLayer layerWithSession: sCaptureSession];
        previewLayer.frame = sPreview.bounds;
        previewLayer.videoGravity = AVLayerVideoGravityResizeAspectFill;
#if __IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_6_0
        BOOL orientationSupported = [[previewLayer connection] isVideoOrientationSupported];
#else
        BOOL orientationSupported = previewLayer.orientationSupported;
#endif /* __IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_6_0 */
        if (orientationSupported) {
#if __IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_6_0
            AVCaptureVideoOrientation newOrientation = [[previewLayer connection] videoOrientation];
#else
            AVCaptureVideoOrientation newOrientation = previewLayer.orientation;
#endif /* __IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_6_0 */
            switch ([UIDevice currentDevice].orientation) {
                case UIInterfaceOrientationPortrait: newOrientation = AVCaptureVideoOrientationPortrait; break;
                case UIInterfaceOrientationPortraitUpsideDown: newOrientation = AVCaptureVideoOrientationPortraitUpsideDown; break;
                case UIInterfaceOrientationLandscapeLeft: newOrientation = AVCaptureVideoOrientationLandscapeLeft; break;
                case UIInterfaceOrientationLandscapeRight: newOrientation = AVCaptureVideoOrientationLandscapeRight; break;
                default:
                case UIDeviceOrientationUnknown:
                case UIDeviceOrientationFaceUp:
                case UIDeviceOrientationFaceDown:
                    break;
            }
#if __IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_6_0
            [previewLayer.connection setVideoOrientation:newOrientation];
#else
            previewLayer.orientation = newOrientation;
#endif /* __IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_6_0 */
        }
        
        [sPreview.layer addSublayer: previewLayer];
        [sCaptureSession startRunning];
    }
    
    return YES;
#else
    return NO;
#endif /* SC_PRODUCER_IOS_HAVE_VIDEO_CAPTURE */
}

@end

#endif /* SC_UNDER_IPHONE */
