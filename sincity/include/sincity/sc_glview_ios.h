#ifndef SINCITY_GLVIEW_IOS_H
#define SINCITY_GLVIEW_IOS_H

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "sincity/sc_annotation.h"

@protocol SCGlviewIOSDelegate <NSObject>
@optional
-(void) glviewAnimationStarted;
-(void) glviewAnimationStopped;
-(void) glviewVideoSizeChanged;
-(void) glviewViewportSizeChanged;
-(void) glviewAnnotationReady:(NSString*)json;
@end

@interface SCGlviewIOS : UIView<SCAnnotationDelegate> {
}

-(void)setFps:(GLuint)fps;
-(void)startAnimation;
-(void)stopAnimation;
-(void)setOrientation:(UIDeviceOrientation)orientation;
-(void)setBufferYUV:(const uint8_t*)buffer width:(uint)bufferWidth height:(uint)bufferHeight;
-(void)setDelegate:(id<SCGlviewIOSDelegate>)delegate;
-(void)setPAR:(int)numerator denominator:(int)denominator;
-(void)setFullscreen:(BOOL)fullscreen;
-(void)setFreeze:(BOOL)freeze;
-(void)setAnnotationType:(SCAnnotationType)annotationType;
-(void)clearAnnotations;
@property(readonly) int viewportX;
@property(readonly) int viewportY;
@property(readonly) int viewportWidth;
@property(readonly) int viewportHeight;
@property(readonly) int videoWidth;
@property(readonly) int videoHeight;
@property(readonly) BOOL animating;
@property(readonly) BOOL freezed;
@end

#endif /* SINCITY_GLVIEW_IOS_H */

