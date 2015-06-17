#import "sincity/sc_annotation.h"
#import "sincity/sc_debug.h"
#import "tsk_string.h"
#import "tsk_memory.h"

#define kTAG "SCAnnotationAny"

typedef struct {
    char* data;
}SCSVGPath;

static void SCCGPathApplierFunc(void *info, const CGPathElement *element) {
    SCSVGPath *svgData = (SCSVGPath *)info;
    
    CGPoint *points = element->points;
    
    switch(element->type) {
        case kCGPathElementMoveToPoint: // #1 point
            tsk_strcat_2(&svgData->data, "M%.2f %.2f", points[0].x, points[0].y);
            break;
        case kCGPathElementAddLineToPoint: // #1 point
            tsk_strcat_2(&svgData->data, "L%.2f %.2f", points[0].x, points[0].y);
            break;
        case kCGPathElementAddQuadCurveToPoint: // #2 points
            tsk_strcat_2(&svgData->data, "Q%.2f %.2f %.2f %.2f", points[0].x, points[0].y, points[1].x, points[1].y);
            break;
        case kCGPathElementAddCurveToPoint: // #3 points
            tsk_strcat_2(&svgData->data, "C%.2f %.2f %.2f %.2f %.2f %.2f", points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y);
            break;
        case kCGPathElementCloseSubpath: // contains #0 point
            tsk_strcat_2(&svgData->data, "Z");
            break;
    }
}

@interface SCAnnotationAny ()
+(long)uniqueID;
@end

@implementation SCAnnotationAny {
    UIView* view_;
    long ID_;
    long localID_;
    NSObject<SCAnnotationDelegate>* delegate_;
    UIColor* strokeColor_;
    UIColor* fillColor_;
    CGFloat strokeWidth_;
}

-(id<SCAnnotation>)initWithView:(UIView*)view {
    assert(view);
    if (self = [super init]) {
        view_ = [view retain];
        ID_ = [[self class] uniqueID];
        localID_ = [[self class] uniqueID];
        strokeColor_ = [[UIColor yellowColor] retain];
        fillColor_ = [[UIColor clearColor] retain];
        strokeWidth_ = 2.0;
    }
    return self;
}

-(void)setDelegate:(id<SCAnnotationDelegate>)_delegate {
    [delegate_ release];
    delegate_ = [_delegate retain];
}

-(void)setStrokeColor:(UIColor*)color {
    [strokeColor_ release];
    strokeColor_ = [color retain];
}

-(void)setStrokeWidth:(CGFloat)width {
    strokeWidth_ = width;
}

-(void)begin:(CGPoint*)pt {
    [NSException raise:NSInternalInconsistencyException
                format:@"You must override %@ in a subclass", NSStringFromSelector(_cmd)];
}

-(void)move:(CGPoint*)pt {
    [NSException raise:NSInternalInconsistencyException
                format:@"You must override %@ in a subclass", NSStringFromSelector(_cmd)];
}

-(void)end:(CGPoint*)pt {
    [NSException raise:NSInternalInconsistencyException
                format:@"You must override %@ in a subclass", NSStringFromSelector(_cmd)];
}

-(void)cancel {
    [NSException raise:NSInternalInconsistencyException
                format:@"You must override %@ in a subclass", NSStringFromSelector(_cmd)];
}

+(long)uniqueID {
    static NSLock* lock_ = nil;
    if (!lock_) {
        lock_ = [[NSLock alloc] init];
    }
    static long uniqueID_ = 0;
    
    [lock_ lock];
    long ID = ++uniqueID_;
    [lock_ unlock];
    return ID;
}

+(NSString*)colorToHexString:(CGColorRef)color {
    CGColorSpaceModel colorSpaceModel = CGColorSpaceGetModel(CGColorGetColorSpace(color));
    CGFloat red = 0.0f, green = 0.0f, blue = 0.0f;
    if ((colorSpaceModel != kCGColorSpaceModelRGB) && (colorSpaceModel != kCGColorSpaceModelMonochrome)) {
        SC_DEBUG_ERROR_EX(kTAG, "Cannot provide RGB components from colorSpaceModel = %d", colorSpaceModel);
    }
    else {
        SC_DEBUG_INFO_EX(kTAG, "colorSpaceModel=%d", colorSpaceModel);
        const CGFloat *comp;
        comp = CGColorGetComponents(color);
        // red
        red = comp[0];
        // green
        green = (colorSpaceModel == kCGColorSpaceModelMonochrome) ? red : comp[1];
        // blue
        blue = (colorSpaceModel == kCGColorSpaceModelMonochrome) ? red : comp[2];
#if 0
        alpha = comp[CGColorGetNumberOfComponents(strokeColor_.CGColor) - 1];
#endif
#if !defined(TSK_CLAMP)
#define TSK_CLAMP(nMin, nVal, nMax)		((nVal) > (nMax)) ? (nMax) : (((nVal) < (nMin)) ? (nMin) : (nVal))
#endif
        red = TSK_CLAMP(0.0f, red, 1.0f);
        green = TSK_CLAMP(0.0f, green, 1.0f);
        blue = TSK_CLAMP(0.0f, blue, 1.0f);
    }
    return [NSString stringWithFormat:@"%02X%02X%02X", (int)(red * 255.f), (int)(green * 255.f), (int)(blue * 255.f)];
}

// http://www.w3.org/TR/SVG/paths.html#PathData
+(NSString*)bezierPathToSVG:(UIBezierPath*)path {
    SCSVGPath svgData = { .data = tsk_null };
    CGPathApply(path.CGPath, &svgData, SCCGPathApplierFunc);
    NSString* nsSvgData = [NSString stringWithFormat:@"%s", svgData.data];
    TSK_FREE(svgData.data);
    return nsSvgData;
}

-(NSString*) json {
    @throw [NSException exceptionWithName:NSInternalInconsistencyException
                                   reason:[NSString stringWithFormat:@"You must override %@ in a subclass", NSStringFromSelector(_cmd)]
                                 userInfo:nil];
}

-(long) ID {
    return ID_;
}

-(long) localID {
    return localID_;
}

-(UIView*) view {
    return view_;
}

-(UIColor*) fillColor {
    return fillColor_;
}

-(UIColor*) strokeColor {
    return strokeColor_;
}

-(NSString*) strokeColorAsHexString {
    return [[self class] colorToHexString:strokeColor_.CGColor];
}

-(CGFloat) strokeWidth {
    return strokeWidth_;
}

-(id<SCAnnotationDelegate>) delegate {
    return delegate_;
}

-(void)dealloc {
    [view_ release], view_ = nil;
    [delegate_ release], delegate_ = nil;
    [strokeColor_ release], strokeColor_ = nil;
    [fillColor_ release], fillColor_ = nil;
    [super dealloc];
}

@end