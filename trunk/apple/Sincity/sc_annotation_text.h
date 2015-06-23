#ifndef SINCITY_ANNOTATION_TEXT_H
#define SINCITY_ANNOTATION_TEXT_H

#import "Sincity/sc_annotation.h"

@interface SCAnnotationText : SCAnnotationAny<UITextFieldDelegate> {
}
-(SCAnnotationText*)initWithView:(UIView*)view;
@end

#endif /* SINCITY_ANNOTATION_TEXT_H */
