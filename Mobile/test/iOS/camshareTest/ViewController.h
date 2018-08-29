//
//  ViewController.h
//  camshareTest
//
//  Created by alex shum on 17/2/23.
//  Copyright © 2017年 alex shum. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <AVFoundation/AVFoundation.h>
#import "CamshareClientOC.h"
#import <CoreGraphics/CoreGraphics.h>
#import <CoreVideo/CoreVideo.h>
#import <CoreMedia/CoreMedia.h>

@interface ViewController : UIViewController<AVCaptureVideoDataOutputSampleBufferDelegate, CamshareClientOCDelegate>
{
    CamshareClientOC *mClientManager;
    
    // 摄像头
    AVCaptureSession *_captureSession;
    UIImageView *_imageView;
    CALayer* _customLayer;
    AVCaptureVideoPreviewLayer* _prevLayer;
    FILE *fp_yuv;
}
@property (weak, nonatomic) IBOutlet UIImageView *videoView1;
@property (weak, nonatomic) IBOutlet UIImageView *videoView2;
@property (strong, nonatomic) UIImageView *videoView3;
//@property (nonatomic, weak) OpenGLView20 *videoOpenView;
//@property (assign, nonatomic) int playWidth;
//@property (assign, nonatomic) int playHeigth;
// 登录
- (IBAction)Btlogin:(id)sender;
// 播放
- (IBAction)Btplay:(id)sender;
// 上传
- (IBAction)Btupload:(id)sender;
// 登出
- (IBAction)Btlogout:(id)sender;

- (void)initCapture;
- (void)startCapture;
- (void)stopCapture;

- (void)playImage:(UIImage*) image;

+ (UIImage *) convertBitmapRGBA8ToUIImage:(unsigned char *) buffer
                                withWidth:(int) width
                               withHeight:(int) height;

- (void)playYuvImage:(const unsigned char *) image withWidth:(int) width withHeight:(int) height;



@end

