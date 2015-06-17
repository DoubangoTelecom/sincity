#import <Foundation/Foundation.h>
#import "sincity/sc_annotation_text.h"
#import "sincity/sc_debug.h"
#import "sincity/jsoncpp/sc_json.h"

#define kTAG "SCAnnotationText"

@interface SCAnnotationTextField : UITextField
@end

@implementation SCAnnotationTextField
#if 0
- (CGSize)intrinsicContentSize {
    if (self.isEditing) {
        CGSize size = [self.text sizeWithAttributes:self.typingAttributes];
        return CGSizeMake(size.width + self.rightView.bounds.size.width + self.leftView.bounds.size.width + 2, size.height);
    }
    return [super intrinsicContentSize];
}
#endif
@end

@interface SCAnnotationText()

@end

@implementation SCAnnotationText {
    UITextField* textField;
    BOOL disableEditing;
}

-(SCAnnotationText*)initWithView:(UIView*)view_ {
    if (self = [super initWithView:view_]) {
        disableEditing = NO;
    }
    return self;
}

-(BOOL)textFieldShouldReturn:(UITextField *)theTextField {
    SC_DEBUG_INFO_EX(kTAG, "textFieldShouldReturn");
    // resign as first responder to end editing
    if (theTextField.editing) {
        if ([theTextField canResignFirstResponder]) {
            [theTextField resignFirstResponder];
        }
    }
    return YES;
}

- (BOOL)textFieldShouldBeginEditing:(UITextField *)textField {
    return !disableEditing;
}

- (void)textFieldDidEndEditing:(UITextField *)theTextField {
    SC_DEBUG_INFO_EX(kTAG, "textFieldDidEndEditing");
    if ([theTextField canResignFirstResponder]) {
        [theTextField resignFirstResponder];
        theTextField.enabled = NO;
        disableEditing = YES;
    }
    if ([theTextField.text length] != 0) {
        if ([super.delegate respondsToSelector:@selector(annotationReady:)]) {
            [super.delegate annotationReady:self];
        }
    }
    else {
        [theTextField removeFromSuperview];
    }
}

-(void)textFieldDidChange :(UITextField *)theTextField{
    CGSize size = [theTextField sizeThatFits:CGSizeMake(FLT_MAX, FLT_MAX)];
    theTextField.frame = CGRectMake(theTextField.frame.origin.x, theTextField.frame.origin.y, size.width, size.height);
#if 0
    [UIView animateWithDuration:0.1 animations:^{
        [theTextField invalidateIntrinsicContentSize];
    }];
#endif
}

-(void)begin:(CGPoint*)pt {
    SC_DEBUG_INFO_EX(kTAG, "TextBegin(%f, %f)", pt->x, pt->y);
    if (textField) {
        if ([textField canResignFirstResponder]) {
            [textField resignFirstResponder];
        }
        [textField release];
    }
#define kPlaceholderText @"type your annotation"
    
    textField = [[SCAnnotationTextField alloc] initWithFrame:CGRectMake(pt->x, pt->y, 1, 1)];
    textField.tag = kAnnotationTextFieldTag;
    textField.borderStyle = UITextBorderStyleNone;
    textField.font = [UIFont systemFontOfSize:15];
    textField.textColor = super.strokeColor;
    textField.backgroundColor = super.fillColor;
    textField.placeholder = kPlaceholderText;
    textField.text = kPlaceholderText;
    textField.autocorrectionType = UITextAutocorrectionTypeDefault;
    textField.keyboardType = UIKeyboardTypeDefault;
    textField.returnKeyType = UIReturnKeyDone;
    textField.clearButtonMode = UITextFieldViewModeWhileEditing;
    [textField setTranslatesAutoresizingMaskIntoConstraints:YES];
    textField.autoresizesSubviews = YES;
    textField.autoresizingMask = UIViewAutoresizingFlexibleWidth;
    textField.contentMode = UIViewContentModeScaleToFill;
    textField.contentVerticalAlignment = UIControlContentVerticalAlignmentCenter;
    textField.contentHorizontalAlignment = UIControlContentHorizontalAlignmentCenter;
    textField.delegate = self;
    [textField addTarget:self action:@selector(textFieldDidChange:) forControlEvents:UIControlEventEditingChanged];
    CGSize defaultSize = [textField sizeThatFits:CGSizeMake(FLT_MAX, FLT_MAX)];
    textField.frame = CGRectMake(textField.frame.origin.x, textField.frame.origin.y - (defaultSize.height / 2), defaultSize.width, defaultSize.height);
    textField.text = @""; // reset text now that default size is computed
    [super.view addSubview:textField];
    if ([textField canBecomeFirstResponder]) {
        [textField becomeFirstResponder];
    }
}

-(void)move:(CGPoint*)pt {
    SC_DEBUG_INFO_EX(kTAG, "TextMove(%f, %f)", pt->x, pt->y);
    
}

-(void)end:(CGPoint*)pt {
    SC_DEBUG_INFO_EX(kTAG, "TextEnd(%f, %f)", pt->x, pt->y);
    // nothing to do, wait for end of editing
}

-(void)cancel {
    SC_DEBUG_INFO_EX(kTAG, "TextCancel()");
    if (textField) {
        [textField removeFromSuperview];
        [textField release];
        textField = nil;
    }
}

-(NSString*) json {
    Json::Value root;
    root["messageType"] = "annotation";
    root["passthrough"] = YES;
    root["id"] = [[NSString stringWithFormat:@"%ld", self.ID] UTF8String];
    root["localId"] = [[NSString stringWithFormat:@"%ld", self.localID] UTF8String];
    root["type"] = "o-text";
    root["hexColor"] =   [[NSString stringWithFormat:@"%@%@", @"#", [[self class] colorToHexString:super.strokeColor.CGColor]]UTF8String];
    root["strokeWidth"] = [[NSString stringWithFormat:@"%.f", super.strokeWidth] UTF8String];
    root["data"] = [textField.text UTF8String];
    root["centerX"];
    root["centerY"];
    root["radius"];
    root["posX"]=[[NSString stringWithFormat:@"%.f", textField.frame.origin.x] UTF8String];;
    root["posY"]=[[NSString stringWithFormat:@"%.f", textField.frame.origin.y] UTF8String];;
    std::string json = root.toStyledString();
    return [NSString stringWithCString:json.c_str() encoding:NSUTF8StringEncoding];
    
}

-(void)dealloc {
    if (textField) {
        if ([textField canResignFirstResponder]) {
            [textField resignFirstResponder];
        }
        [textField release], textField = nil;
    }
    [super dealloc];
}

@end