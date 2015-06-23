#import <Foundation/Foundation.h>
#import "sc_annotation_freehand.h"
#import "sc_debug.h"
#import "jsoncpp/sc_json.h"

#define kTAG "SCAnnotationFreehand"

@interface SCAnnotationFreehand()
-(void)draw;
@end

@implementation SCAnnotationFreehand {
    CAShapeLayer *layer;
    UIBezierPath* bezierPath;
}

-(SCAnnotationFreehand*)initWithView:(UIView*)view_ {
    if (self = [super initWithView:view_]) {
    }
    return self;
}

-(void)draw {
    if (layer) {
        [layer removeFromSuperlayer];
        [layer release];
    }
    
    layer = [[CAShapeLayer alloc] init];
    layer.name = @"SCAnnotationFreehand";
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
    SC_DEBUG_INFO_EX(kTAG, "FreehandBegin(%f, %f)", pt->x, pt->y);
    if (bezierPath) {
        [bezierPath release];
    }
    pt->x = lroundf(pt->x);
    pt->y = lroundf(pt->y);
    bezierPath = [[UIBezierPath alloc] init];
    bezierPath.lineCapStyle = kCGLineCapRound;
    bezierPath.lineJoinStyle = kCGLineJoinRound;
    bezierPath.miterLimit = -10;
    bezierPath.flatness = 0.0;
    bezierPath.lineWidth = super.strokeWidth;
    [bezierPath moveToPoint:*pt];
    [self draw];
}

-(void)move:(CGPoint*)pt {
    SC_DEBUG_INFO_EX(kTAG, "FreehandMove(%f, %f)", pt->x, pt->y);
    pt->x = lroundf(pt->x);
    pt->y = lroundf(pt->y);
    [bezierPath addLineToPoint:*pt];
    [self draw];
}

-(void)end:(CGPoint*)pt {
    SC_DEBUG_INFO_EX(kTAG, "FreehandEnd(%f, %f)", pt->x, pt->y);
    pt->x = lroundf(pt->x);
    pt->y = lroundf(pt->y);
    [bezierPath addLineToPoint:*pt];
    [self draw];
    if ([super.delegate respondsToSelector:@selector(annotationReady:)]) {
        [super.delegate annotationReady:self];
    }
}

-(void)cancel {
    SC_DEBUG_INFO_EX(kTAG, "FreehandCancel()");
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
    root["type"] = "o-path";
    root["hexColor"] = [[NSString stringWithFormat:@"%@%@", @"#", [[self class] colorToHexString:layer.strokeColor]]UTF8String];
    root["strokeWidth"] = [[NSString stringWithFormat:@"%.f", layer.lineWidth] UTF8String];
    root["data"]=[[[self class] bezierPathToSVG:bezierPath] UTF8String];;
    root["centerX"];
    root["centerY"];
    root["radius"];
    root["posX"];
    root["posY"];
    std::string json = root.toStyledString();
    return [NSString stringWithCString:json.c_str() encoding: NSUTF8StringEncoding];
    
}

-(void)dealloc {
    [layer release], layer = nil;
    [bezierPath release], bezierPath = nil;
    [super dealloc];
}

@end