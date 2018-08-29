//
//  ViewController.m
//  camshareTest
//
//  Created by alex shum on 17/2/23.
//  Copyright © 2017年 alex shum. All rights reserved.
//

#import "ViewController.h"

#import <CoreVideo/CoreVideo.h>

@interface ViewController ()

@end
static ViewController* gManager = nil;
static int playWidth = 0;
static int playHeigth = 0;


static void decodeYUV420SP(unsigned char* rgb, const unsigned char* yuv420sp, int width, int height) {
    int frameSize = width * height;
    for (int j = 0, yp = 0; j < height; j++) {
        int uvp = frameSize + (j >> 1) * width/2, u = 0, v = 0;
        int vup = frameSize + frameSize/4 + (j >> 1) * width/2;
        for (int i = 0; i < width; i++, yp++) {
            int y = (0xff & ((int) yuv420sp[yp]));
            if (y < 0) y = 0;
            if ((i & 1) == 0) {
                
                u = (0xff & yuv420sp[uvp++]);
                v = (0xff & yuv420sp[vup++]);
                
            }
            int r = y + v + (v*103>>8);
            int g = y - (u*88>>8) - (v*183>>8);
            int b = y + u + (u*198>>8);
            
            r = r>255?255:(r<0?0:r);
            g = g>255?255:(g<0?0:g);
            b = b>255?255:(b<0?0:b);
            
            rgb[yp*2] = (((g & 0x1C) << 3) | (b >> 3));
            rgb[yp*2 + 1] = ((r & 0xF8) | (g>>5));
            
        }
    }
}


static void rgb565_to_rgb24(unsigned short* rgb565, unsigned char *rgb24, int width, int height)
{
//    int x =0, y = 0;
//    for (y = 0; y < height; y++) {
//        for (x = 0; x < width; x++) {
//            int r = ((*(rgb565 + y*width + x) & 0xf800) >> 8) & 0xff;
//            int g = ((*(rgb565 + y*width + x) & 0x07e0) >> 3) & 0xff;
//            int b = ((*(rgb565 + y*width + x) & 0x001f) << 8) & 0xff;
//           // NSLog(@"rgb565_to_rgb24 r:%d g:%d b:%d", r, g, b);
//            *(rgb24 + x*3 + 0) = r;
//            *(rgb24 + x*3 + 1) = g;
//            *(rgb24 + x*3 + 2) = b;
//        }
//    }
 
    int x =0, y = 0;
    for (y = 0; y < height; y++) { 
        for (x = 0; x < width; x++) {
            int r = (rgb565[2 * (y*width + x) + 1] & 0xf8);
            int g = ((rgb565[2 * (y*width + x) + 1] & 0x07) << 4) + ((rgb565[2 * (y*width + x)] & 0xe0) >> 4);
            int b = rgb565[2 * (y*width + x)] & 0x1f;
            //NSLog(@"rgb565_to_rgb24 r:%d g:%d b:%d", r, g, b);
            rgb24[3 * (y*width + x) + 0] = r;
            rgb24[3 * (y*width + x) + 1] = g;
            rgb24[3 * (y*width + x) + 2] = b;
        }
    }

}

static void rgb24_to_rgb565(unsigned char *rgb24, unsigned char *rgb16, int width, int height)
{
    int i = 0, j = 0;
    for (i = 0; i < width*height*3; i += 3)
    {
        rgb16[j] = rgb24[i] >> 3; // B
        rgb16[j] |= ((rgb24[i+1] & 0x1C) << 3); // G
        rgb16[j+1] = rgb24[i+2] & 0xF8;// R
        rgb16[j+1] |= (rgb24[i+1] >>5); // G
        j += 2;
    }
}


static unsigned char* detailPic0(unsigned char* data, int imageWidth, int imageHeight, int newImageW, int newImageH) {
    unsigned char* yuv_temp =new unsigned char[newImageW*newImageH*3/2];
    int deleteW = (imageWidth - newImageW) / 2;
    int deleteH = (imageHeight - newImageH) / 2;
    //处理y 旋转加裁剪
    int i, j;
    int index = 0;
    for (j = deleteH; j < imageHeight- deleteH; j++) {
        for (i = deleteW; i < imageWidth- deleteW; i++)
            yuv_temp[index++]= data[j * imageWidth + i];
    }
    
    //处理u
    index= newImageW * newImageH;
    
    for (i = imageHeight + deleteH / 2;i < imageHeight / 2 * 3 - deleteH / 2; i++)
        for (j = deleteW + 1; j< imageWidth - deleteW; j += 2)
            yuv_temp[index++]= data[i * imageWidth + j];
    
    //处理v 旋转裁剪加格式转换
    for (i = imageHeight + deleteH / 2;i < imageHeight / 2 * 3 - deleteH / 2; i++)
        for (j = deleteW; j < imageWidth - deleteW; j += 2)
            yuv_temp[index++]= data[i * imageWidth + j];
    return yuv_temp;
}

//旋转90度，裁剪，NV21转YUV420P（竖着录制后摄像头）
static unsigned char* detailPic90(unsigned char* data, int imageWidth, int imageHeight, int newImageW, int newImageH) {
    
    unsigned char* yuv_temp =new unsigned char[newImageW*newImageH*3/2];
    int deleteW = (imageWidth - newImageH) / 2;
    int deleteH = (imageHeight - newImageW) / 2;
    
    int i, j;
    
    for (i = 0; i < newImageH; i++){
        for (j = 0; j < newImageW; j++){
            yuv_temp[(newImageH- i) * newImageW - 1 - j] = data[imageWidth * (deleteH + j) + imageWidth - deleteW
                                                                -i];
        }
    }
    
    int index = newImageW * newImageH;
    for (i = deleteW + 1; i< imageWidth - deleteW; i += 2)
        for (j = imageHeight / 2 * 3 -deleteH / 2; j > imageHeight + deleteH / 2; j--)
            yuv_temp[index++]= data[(j - 1) * imageWidth + i];
    
    for (i = deleteW; i < imageWidth- deleteW; i += 2)
        for (j = imageHeight / 2 * 3 -deleteH / 2; j > imageHeight + deleteH / 2; j--)
            yuv_temp[index++]= data[(j - 1) * imageWidth + i];
    return yuv_temp;
}



@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view, typically from a nib.
    //gListener = new PCamshareClientListener();
    // 1.创建一个camshare客服端管理器
    mClientManager = [CamshareClientOC instance];
    
    char input_str_full2[500]={0};
    NSString *Logpath = [self fileCreate:@"log"];
    sprintf(input_str_full2,"%s",[Logpath UTF8String]);
    printf("Input Path:%s\n",input_str_full2);

    
    char input_str_full[500]={0};
    NSString *path = [self fileCreate:@"myInfo.yuv"];
    sprintf(input_str_full,"%s",[path UTF8String]);
    printf("Input Path:%s\n",input_str_full);

    
    _imageView = nil;
    _prevLayer = nil;
    _customLayer = nil;
    

    gManager = self;
    
//    self.videoOpenView = [mClientManager CreateOpenGLView:_videoView2.frame];
//    
//    _videoOpenView = [[OpenGLView20 alloc] initWithFrame:_videoView2.frame];
//    [self.videoOpenView setVideoSize:352 height:288];
//    [self.view addSubview:self.videoOpenView];
    mClientManager.delegate = self;
    self.videoView3 = [[UIImageView alloc] init];
    self.videoView3.frame = _videoView2.frame;
    [self.videoView3 setContentMode:UIViewContentModeCenter];
    [self.view addSubview:self.videoView3];
    OpenGLView20* open = [mClientManager CreateOpenGLView:_videoView2.frame];
    [self.view addSubview:open];
    
}


- (NSString *)fileCreate:(NSString *)path {
    // 1.创建文件管理器
    NSFileManager *fileManager = [NSFileManager defaultManager];
    
    // 2.获取document路径,括号中属性为当前应用程序独享
    NSArray *directoryPaths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentDirectory = [directoryPaths objectAtIndex:0];
    
    // 3.定义记录文件全名以及路径的字符串filePath
    NSString *filePath = [documentDirectory stringByAppendingPathComponent:path];
    NSLog(@"filePath=%@", filePath);
    
    // 4.查找文件，如果不存在，就创建一个文件
    if (![fileManager fileExistsAtPath:filePath]) {
        [fileManager createFileAtPath:filePath contents:nil attributes:nil];
    }
    
    return filePath;
    
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


- (IBAction)Btlogin:(id)sender {
    NSString* hostName = @"52.196.96.7";
    NSString* user = @"WW1";
    NSString* password = @"";
    NSString* site = @"1";
    NSString* sid = @"SESSION123456";

    UserType type= UserType_Man;
    [mClientManager start:hostName user:user password:password site:site sid:sid userType:type];

}

- (IBAction)Btplay:(id)sender {
    NSString* destId = @"MM1";
    NSString* serverId = @"PC0";
//    //7. makecall进入camshare室，pDestId 与 login的user相同就是上传，否则是下载(这里是下载)
    [mClientManager makeCall:destId serverId:serverId];
}

- (IBAction)Btupload:(id)sender {
    NSString* destId = @"MM1";
    NSString* serverId = @"PC0";
    [mClientManager makeCall:destId serverId:serverId];
    [mClientManager setVideoSize:176 height:144];
    [mClientManager setVideRate:6];
    [mClientManager  chooseDecoderVideoFramte:VIDEO_FORMATE_RGB24];
    
    [self initCapture];
    [self startCapture];

}

- (IBAction)Btlogout:(id)sender {
   // [mClientManager stop:YES];
}


- (AVCaptureDevice *)getCameraDevice:(BOOL)isFront
{
    NSArray *cameras = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
    
    AVCaptureDevice *frontCamera;
    AVCaptureDevice *backCamera;
    
    for (AVCaptureDevice *camera in cameras) {
        if (camera.position == AVCaptureDevicePositionBack) {
            backCamera = camera;
        } else {
            frontCamera = camera;
        }
    }
    
    if (isFront) {
        return frontCamera;
    }
    return backCamera;
}
- (void)initCapture
{
    _captureSession = [[AVCaptureSession alloc] init];
    _captureSession.sessionPreset = AVCaptureSessionPreset352x288;
    AVCaptureDevice *device = [self getCameraDevice:YES];
    AVCaptureDeviceInput *captureInput =  [AVCaptureDeviceInput deviceInputWithDevice:device error:nil];
    [_captureSession addInput:captureInput];
    
    AVCaptureVideoDataOutput *captureOutput = [[AVCaptureVideoDataOutput alloc] init];
//    // 10.选择摄像头采集的格式（这里专用了ios）
    int pixelFormatType = (int)[mClientManager chooseEncoderVideoFramte:captureOutput.availableVideoCVPixelFormatTypes];
    NSDictionary *settings = [[NSDictionary alloc] initWithObjectsAndKeys:[NSNumber numberWithUnsignedInt:pixelFormatType],kCVPixelBufferPixelFormatTypeKey,nil];
    

    
    captureOutput.videoSettings = settings;
    captureOutput.alwaysDiscardsLateVideoFrames = YES;
    
    
    
    dispatch_queue_t queue = dispatch_queue_create("myEncoderH264Queue", NULL);
    [captureOutput setSampleBufferDelegate:self queue:queue];
    if ([_captureSession canAddOutput:captureOutput]) {
        [_captureSession addOutput:captureOutput];
    }
    
    NSArray *array = [[_captureSession.outputs objectAtIndex:0] connections];
    for (AVCaptureConnection *connection in array)
    {
        connection.videoOrientation = AVCaptureVideoOrientationPortrait;
    }
    
    _prevLayer = [AVCaptureVideoPreviewLayer layerWithSession:_captureSession];
    _prevLayer.frame = self.videoView1.frame;
    _prevLayer.videoGravity = AVLayerVideoGravityResizeAspect;
    [[_prevLayer connection] setVideoOrientation:AVCaptureVideoOrientationPortrait];
    [self.videoView1.layer addSublayer:_prevLayer];
    // [self startCapture];
}

- (void)startCapture
{
    [_captureSession startRunning];
}

- (void)stopCapture
{
    [_captureSession stopRunning];
}

- (void)playImage:(UIImage*) image
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [_videoView2 setImage:image];
    });
}

- (void)playYuvImage:(const unsigned char *) image withWidth:(int) width withHeight:(int) height
{
    //[_videoOpenView displayYUV420pData:(void *)image width:width height:height];
}


+ (UIImage *) convertBitmapRGBA8ToUIImage:(unsigned char *) buffer
                                withWidth:(int) width
                               withHeight:(int) height {
    
    // added code
    char* rgba = (char*)malloc(width*height*4);
    for(int i=0; i < width*height; ++i) {
        rgba[4*i] = buffer[3*i];
        rgba[4*i+1] = buffer[3*i+1];
        rgba[4*i+2] = buffer[3*i+2];
        rgba[4*i+3] = 255;
    }
    //
    
    
    size_t bufferLength = width * height * 4;
    CGDataProviderRef provider = CGDataProviderCreateWithData(NULL, rgba, bufferLength, NULL);
    size_t bitsPerComponent = 8;
    size_t bitsPerPixel = 32;
    size_t bytesPerRow = 4 * width;
    
    CGColorSpaceRef colorSpaceRef = CGColorSpaceCreateDeviceRGB();
    if(colorSpaceRef == NULL) {
        NSLog(@"Error allocating color space");
        CGDataProviderRelease(provider);
        return nil;
    }
    
    CGBitmapInfo bitmapInfo = kCGBitmapByteOrderDefault | kCGImageAlphaPremultipliedLast;
    CGColorRenderingIntent renderingIntent = kCGRenderingIntentDefault;
    
    CGImageRef iref = CGImageCreate(width,
                                    height,
                                    bitsPerComponent,
                                    bitsPerPixel,
                                    bytesPerRow,
                                    colorSpaceRef,
                                    bitmapInfo,
                                    provider,   // data provider
                                    NULL,       // decode
                                    YES,            // should interpolate
                                    renderingIntent);
    
    uint32_t* pixels = (uint32_t*)malloc(bufferLength);
    
    if(pixels == NULL) {
        NSLog(@"Error: Memory not allocated for bitmap");
        CGDataProviderRelease(provider);
        CGColorSpaceRelease(colorSpaceRef);
        CGImageRelease(iref);
        return nil;
    }
    
    CGContextRef context = CGBitmapContextCreate(pixels,
                                                 width,
                                                 height,
                                                 bitsPerComponent,
                                                 bytesPerRow,
                                                 colorSpaceRef,
                                                 bitmapInfo);
    
    if(context == NULL) {
        NSLog(@"Error context not created");

    }
    
    UIImage *image = nil;
    if(context) {
        
        CGContextDrawImage(context, CGRectMake(0.0f, 0.0f, width, height), iref);
        
        CGImageRef imageRef = CGBitmapContextCreateImage(context);
        
        // Support both iPad 3.2 and iPhone 4 Retina displays with the correct scale
        if([UIImage respondsToSelector:@selector(imageWithCGImage:scale:orientation:)]) {
            float scale = [[UIScreen mainScreen] scale];
            image = [UIImage imageWithCGImage:imageRef scale:scale orientation:UIImageOrientationUp];
        } else {
            image = [UIImage imageWithCGImage:imageRef];
        }
        
        CGImageRelease(imageRef);
        CGContextRelease(context);
    }
    
    CGColorSpaceRelease(colorSpaceRef);
    CGImageRelease(iref);
    CGDataProviderRelease(provider);
    
    if(pixels) {
        free(pixels);
    }
    return image;
}


#pragma mark -- AVCaptureVideoDataOutputSampleBufferDelegate method
- (void)captureOutput:(AVCaptureOutput *)captureOutput didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer fromConnection:(AVCaptureConnection *)connection
{
    CVPixelBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
    if (CVPixelBufferLockBaseAddress(imageBuffer, 0) == kCVReturnSuccess) {
        //void *imageAddress =        CVPixelBufferGetBaseAddressOfPlane(imageBuffer, 0);
        //UInt8 *bufferbasePtr = (UInt8 *)CVPixelBufferGetBaseAddress(imageBuffer);
        UInt8 *bufferPtr = (UInt8 *)CVPixelBufferGetBaseAddressOfPlane(imageBuffer,0);
        UInt8 *bufferPtr1 = (UInt8 *)CVPixelBufferGetBaseAddressOfPlane(imageBuffer,1);
        //size_t buffeSize = CVPixelBufferGetDataSize(imageBuffer);
        size_t width = CVPixelBufferGetWidth(imageBuffer);
        size_t height = CVPixelBufferGetHeight(imageBuffer);
        //size_t bytesPerRow = CVPixelBufferGetBytesPerRow(imageBuffer);
        //size_t bytesrow0 = CVPixelBufferGetBytesPerRowOfPlane(imageBuffer,0);
        //size_t bytesrow1  = CVPixelBufferGetBytesPerRowOfPlane(imageBuffer,1);
        //size_t bytesrow2 = CVPixelBufferGetBytesPerRowOfPlane(imageBuffer,2);
        UInt8 *yuv420 = (UInt8 *)malloc(width * height * 3/2);
        int pixelFormat = CVPixelBufferGetPixelFormatType(imageBuffer);
        switch (pixelFormat) {
            case kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange:
                NSLog(@"Capture pixel format=NV12");
                break;
            case kCVPixelFormatType_422YpCbCr8:
                NSLog(@"Capture pixel format=UYUY422");
                break;
            default:
                NSLog(@"Capture pixel format=RGB32");
                break;
        }
        
        memcpy(yuv420, bufferPtr, width * height);
        memcpy(yuv420 + width * height, bufferPtr1, width * height/2);
//        for (int i = 0; i < width * height/2; i+=2) {
//            yuv420[width * height + i+1] = bufferPtr1[i];
//        }
//        for (int i = 1; i < width * height/2 + 2; i+=2) {
//            yuv420[width * height + i-1] = bufferPtr1[i];
//        }

       //BOOL orient = connection.supportsVideoOrientation;
       //AVCaptureVideoOrientation Orientation = connection.videoOrientation;

        NSData *data = [NSData dataWithBytes:yuv420 length:width * height * 3/2];
        [mClientManager insertVideoData:data len:width * height * 3 / 2 timestamp:20 width:width heigh:height direction:Direction_0];

        
        free(yuv420);
    }
    CVPixelBufferUnlockBaseAddress(imageBuffer, 0);

    
}

// 成功完全接收都一个请求后的回调
//-(void)CamshareClientManagerDelegate:(UIImage *)image
//{
//    dispatch_async(dispatch_get_main_queue(), ^{
//    [self.videoView2 setImage:image];
//    
//    });
//}

/**
 *  登录回调
 *
 *  @param client   实例
 *  @param success  登录结果
 */
- (void)onLogin:(CamshareClientOC * _Nonnull)client success:(BOOL)success
{
    NSLog(@"onLogin success:%d",success);
}

/**
 *  断开回调
 *
 *  @param client   实例
 */
- (void)onDisconnect:(CamshareClientOC * _Nonnull)client
{
    NSLog(@"onDisconnect");
}

/**
 *  拨号回调
 *
 *  @param client   实例
 *  @param success  拨号结果
 */
- (void)onMakeCall:(CamshareClientOC * _Nonnull)client success:(BOOL)success
{
    NSLog(@"onMakeCall success:%d",success);
}

/**
 *  挂断回调
 *
 *  @param client   实例
 *  @param cause  挂断原因
 */
- (void)onHangup:(CamshareClientOC * _Nonnull)client cause:(NSString* _Nullable)cause
{
    NSLog(@"onHangup cause:%@",cause);
}

/**
 *  接收视频回调
 *
 *  @param client       实例
 *  @param image       视频图片
 *  @param timeStamp    时间
 */
- (void)onRecvVideo:(CamshareClientOC * _Nonnull)client withUIImage:(UIImage* _Nullable)image withTime:(NSInteger) timeStamp
{
    NSLog(@"onRecvVideo");
    dispatch_async(dispatch_get_main_queue(), ^{
      
        self.videoView2.image = nil;
        self.videoView2.image = image;

    });
}

/**
 *  上传剪切视频回调
 *
 *  @param client       实例
 *  @param image       视频图片
 *  @param timeStamp    时间
 */
- (void)onCallVideo:(CamshareClientOC * _Nonnull)client withUIImage:(UIImage* _Nullable)image withTime:(NSInteger) timeStamp
{
    NSLog(@"onCallVideo");
   // __block UIImage *weakImage = image;
    dispatch_async(dispatch_get_main_queue(), ^{

        self.videoView2.image = nil;
        self.videoView2.image = image;

    });
}


@end
