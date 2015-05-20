#ifndef SINCITY_ANNOTATION_H
#define SINCITY_ANNOTATION_H

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#if !defined(kAnnotationTextFieldTag)
#   define kAnnotationTextFieldTag 6543278
#endif /* kAnnotationTextFieldTag */

typedef enum {
    SCAnnotationTypeNone,
    SCAnnotationTypeCircle,
    SCAnnotationTypeArrow,
    SCAnnotationTypeText,
    SCAnnotationTypeFreehand
}
SCAnnotationType;

@protocol SCAnnotation;

@protocol SCAnnotationDelegate <NSObject>
@optional
-(BOOL) annotationReady:(id<SCAnnotation>)annotation;
-(BOOL) annotationGotPath:(UIBezierPath*)path;
@end

@protocol SCAnnotation <NSObject>
@required
-(id<SCAnnotation>)initWithView:(UIView*)view;
-(void)setDelegate:(id<SCAnnotationDelegate>)delegate;
-(void)setStrokeColor:(UIColor*)color;
-(void)setStrokeWidth:(CGFloat)width;
-(void)begin:(CGPoint*)pt;
-(void)move:(CGPoint*)pt;
-(void)end:(CGPoint*)pt;
-(void)cancel;
@property(readonly) NSString* json;
@end

@interface SCAnnotationAny : NSObject<SCAnnotation> {
}
+(NSString*)colorToHexString:(CGColorRef)color;
+(NSString*)bezierPathToSVG:(UIBezierPath*)path;
@property(readonly) long ID;
@property(readonly) long localID;
@property(readonly) UIView* view;
@property(readonly) UIColor* fillColor;
@property(readonly) UIColor* strokeColor;
@property(readonly) NSString* strokeColorAsHexString;
@property(readonly) CGFloat strokeWidth;
@property(readonly) id<SCAnnotationDelegate> delegate;
@end

#endif /* SINCITY_ANNOTATION_H */
