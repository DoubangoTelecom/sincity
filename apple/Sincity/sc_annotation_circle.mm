#import <Foundation/Foundation.h>
#import "sc_annotation_circle.h"
#import "sc_debug.h"
#import "jsoncpp/sc_json.h"

#define kTAG "SCAnnotationCircle"

@interface SCAnnotationCircle()
-(void)draw;
@end

@implementation SCAnnotationCircle {
    CAShapeLayer *layer;
    CGFloat radius, beginX, beginY;
}

-(SCAnnotationCircle*)initWithView:(UIView*)view_ {
    if (self = [super initWithView:view_]) {
        radius = beginX = beginY = 0.0;
    }
    return self;
}

-(void)draw {
    if (layer) {
        [layer removeFromSuperlayer];
        [layer release];
    }
    
    layer = [[CAShapeLayer alloc] init];
    layer.name = @"SCAnnotationCircle";
    layer.position = CGPointMake((beginX - radius), (beginY - radius)); // (beginX, beginY) will be the center of the circle
    layer.lineWidth = super.strokeWidth;
    UIBezierPath* bezierPath = [UIBezierPath bezierPathWithRoundedRect:CGRectMake(0.0, 0.0, 2.0*radius, 2.0*radius) cornerRadius:radius];
    layer.path = bezierPath.CGPath;
    layer.strokeColor = super.strokeColor.CGColor;
    layer.fillColor = super.fillColor.CGColor;
    
    [[super.view layer] addSublayer:layer];
    
    if ([super.delegate respondsToSelector:@selector(annotationGotPath:)]) {
        [super.delegate annotationGotPath:bezierPath];
    }
}

-(void)begin:(CGPoint*)pt {
    SC_DEBUG_INFO_EX(kTAG, "CircleBegin(%f, %f)", pt->x, pt->y);
    beginX = pt->x;
    beginY = pt->y;
    radius = 0.0;
    [self draw];
}

-(void)move:(CGPoint*)pt {
    SC_DEBUG_INFO_EX(kTAG, "CircleMove(%f, %f)", pt->x, pt->y);
    radius = sqrt(pow(beginX-pt->x,2)+pow(beginY-pt->y,2)); // compute distance between two points
    [self draw];
}

-(void)end:(CGPoint*)pt {
    SC_DEBUG_INFO_EX(kTAG, "CircleEnd(%f, %f)", pt->x, pt->y);
    radius = sqrt(pow(beginX-pt->x,2)+pow(beginY-pt->y,2)); // compute distance between two points
    [self draw]; // draw or clear(radius is nil)
    if (radius > 0.0) {
        if ([super.delegate respondsToSelector:@selector(annotationReady:)]) {
            [super.delegate annotationReady:self];
        }
    }
}

-(void)cancel {
    SC_DEBUG_INFO_EX(kTAG, "CircleCancel()");
    if (layer) {
        [layer removeFromSuperlayer];
        [layer release], layer = nil;
    }
}

-(NSString*) json {
    Json::Value root;
    root["messageType"] = "annotation";
    root["passthrough"] = YES;
    root["id"] = [[NSString stringWithFormat:@"%ld", self.ID] UTF8String];
    root["localId"] = [[NSString stringWithFormat:@"%ld", self.localID] UTF8String];
    root["type"] = "o-circle";
    root["hexColor"] = [[NSString stringWithFormat:@"%@%@", @"#", [[self class] colorToHexString:layer.strokeColor]]UTF8String];
    root["strokeWidth"] = [[NSString stringWithFormat:@"%.f", layer.lineWidth] UTF8String];
    root["data"];
    root["centerX"]=[[NSString stringWithFormat:@"%.f", beginX] UTF8String];
    root["centerY"]=[[NSString stringWithFormat:@"%.f", beginY] UTF8String];
    root["radius"]=[[NSString stringWithFormat:@"%.f", radius] UTF8String];
    root["posX"];
    root["posY"];
    std::string json = root.toStyledString();
    return [NSString stringWithCString:json.c_str() encoding: NSUTF8StringEncoding];
}

-(void)dealloc {
    [layer release], layer = nil;
    [super dealloc];
}

@end