#ifndef SINCITY_CAMERA_IOS_H
#define SINCITY_CAMERA_IOS_H

#include "sc_config.h"

#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

#if SC_UNDER_IPHONE

@interface SCCameraIOS : NSObject {
    
}

#if SC_PRODUCER_IOS_HAVE_VIDEO_CAPTURE
+(AVCaptureDevice *)frontFacingCamera;
+(AVCaptureDevice *)backCamera;
#endif /* SC_PRODUCER_IOS_HAVE_VIDEO_CAPTURE */

+(BOOL)setPreview:(UIView*)preview;
@end

#endif /* SC_UNDER_IPHONE */

#endif /* SINCITY_CAMERA_IOS_H */
