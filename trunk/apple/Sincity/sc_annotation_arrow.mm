#import <Foundation/Foundation.h>
#import "sc_annotation_arrow.h"
#import "sc_debug.h"
#import "jsoncpp/sc_json.h"

#define kTAG "SCAnnotationArrow"

#if !defined(kArrowWingLength)
#   define kArrowWingLength (10.0f)
#endif /* kArrowWingLength */
#if !defined(kArrowWingAngle)
#   define kArrowWingAngle (M_PI/4)
#endif /* kArrowWingAngle */

@interface SCAnnotationArrow()
-(void)draw:(BOOL)lastPoint;
@end

@implementation SCAnnotationArrow {
    CAShapeLayer *layer;
    UIBezierPath* bezierPath;
    CGPoint beginPoint, endPoint;
}

-(SCAnnotationArrow*)initWithView:(UIView*)view_ {
    if (self = [super initWithView:view_]) {
    }
    return self;
}

-(void)draw:(BOOL)lastPoint {
    if (layer) {
        [layer removeFromSuperlayer];
        [layer release];
    }
    
    if (lastPoint) {
        UIBezierPath* arrowPath = [UIBezierPath bezierPath];
        // compute line angle % x-axis
        CGPoint p = CGPointMake(endPoint.x - beginPoint.x, endPoint.y - beginPoint.y);
        CGFloat lineAngle = atan2f(p.y, p.x);
        CGFloat lineLength = sqrt(pow(p.x, 2) + pow(p.y, 2));
        
        // wings must be at most line/3
        CGFloat wingsLenthMax = lineLength / 3;
        CGFloat wingsLenth = kArrowWingLength > wingsLenthMax ? wingsLenthMax : kArrowWingLength;
        
        // move by D(arrow wing length) with angle % (x-axis +- wingAngle)
        p.x = endPoint.x + wingsLenth * cos(lineAngle + M_PI - kArrowWingAngle);
        p.y = endPoint.y + wingsLenth * sin(lineAngle + M_PI - kArrowWingAngle);
        [arrowPath moveToPoint:p];
        [arrowPath addLineToPoint:endPoint];
        p.x = endPoint.x + wingsLenth * cos(lineAngle + M_PI + kArrowWingAngle);
        p.y = endPoint.y + wingsLenth * sin(lineAngle + M_PI + kArrowWingAngle);
        [arrowPath addLineToPoint:p];
        // append arrow path to the line
        [bezierPath appendPath:arrowPath];
    }
    
    layer = [[CAShapeLayer alloc] init];
    layer.name = @"SCAnnotationArrow";
    layer.position = CGPointMake(0.0, 0.0);
    layer.lineWidth = super.strokeWidth;
    layer.path = bezierPath.CGPath;
    layer.strokeColor = super.strokeColor.CGColor;
    layer.fillColor = super.fillColor.CGColor;
    
    [[super.view layer] addSublayer:layer];
    
    if ([super.delegate respondsToSelector:@selector(annotationGotPath:)]) {
        [super.delegate annotationGotPath:bezierPath];
    }
}

-(void)begin:(CGPoint*)pt {
    beginPoint = endPoint = *pt;
    SC_DEBUG_INFO_EX(kTAG, "ArrowBegin(%f, %f)", endPoint.x, endPoint.y);
    if (bezierPath) {
        [bezierPath release];
    }
    
    bezierPath = [[UIBezierPath alloc] init];
    bezierPath.lineCapStyle = kCGLineCapRound;
    bezierPath.lineJoinStyle = kCGLineJoinRound;
    bezierPath.miterLimit = -10;
    bezierPath.flatness = 0.0;
    bezierPath.lineWidth = super.strokeWidth;
    [bezierPath moveToPoint:beginPoint];
    [self draw:NO];
}

-(void)move:(CGPoint*)pt {
    endPoint = *pt;
    SC_DEBUG_INFO_EX(kTAG, "ArrowMove(%f, %f)", endPoint.x, endPoint.y);
    [bezierPath removeAllPoints];
    [bezierPath moveToPoint:beginPoint];
    [bezierPath addLineToPoint:endPoint];
    [self draw:NO];
}

-(void)end:(CGPoint*)pt {
    endPoint = *pt;
    SC_DEBUG_INFO_EX(kTAG, "ArrowEnd(%f, %f)", endPoint.x, endPoint.y);
    [bezierPath removeAllPoints];
    [bezierPath moveToPoint:beginPoint];
    [bezierPath addLineToPoint:endPoint];
    [self draw:YES];
    if (beginPoint.x != endPoint.x || beginPoint.y != endPoint.y) {
        if ([super.delegate respondsToSelector:@selector(annotationReady:)]) {
            [super.delegate annotationReady:self];
        }
    }
}

-(void)cancel {
    SC_DEBUG_INFO_EX(kTAG, "ArrowdCancel()");
    if (layer) {
        [layer removeFromSuperlayer];
        [layer release];
        layer = nil;
    }
}

-(NSString*) json {
    Json::Value root;
    root["messageType"] = "annotation";
    root["passthrough"] = YES;
    root["id"] = [[NSString stringWithFormat:@"%ld", self.ID] UTF8String];
    root["localId"] = [[NSString stringWithFormat:@"%ld", self.localID] UTF8String];
    root["type"] = "o-arrow";
    root["hexColor"] =   [[NSString stringWithFormat:@"%@%@", @"#", [[self class] colorToHexString:layer.strokeColor]]UTF8String];
    root["strokeWidth"] = [[NSString stringWithFormat:@"%.f", layer.lineWidth] UTF8String];
    root["data"] = [[[self class] bezierPathToSVG:bezierPath] UTF8String];
    root["centerX"];
    root["centerY"];
    root["radius"];
    root["posX"];
    root["posY"];
    std::string json = root.toStyledString();
    return [NSString stringWithCString:json.c_str() encoding:NSUTF8StringEncoding];
}

-(void)dealloc {
    [layer release], layer = nil;
    [bezierPath release], bezierPath = nil;
    [super dealloc];
}

@end