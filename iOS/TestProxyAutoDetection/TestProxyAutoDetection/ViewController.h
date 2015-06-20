//
//  ViewController.h
//  TestProxyAutoDetection
//
//  Created by Mamadou DIOP on 09/06/15.
//  Copyright (c) 2015 Doubango Telecom. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface ViewController : UIViewController {
    IBOutlet UIButton *buttonResolve;
    IBOutlet UITextField* textFieldUrl;
}

-(IBAction) onButtonUp:(id)sender;

@property (retain, nonatomic) IBOutlet UIButton *buttonResolve;
@property (retain, nonatomic) IBOutlet UITextField* textFieldUrl;

@end

