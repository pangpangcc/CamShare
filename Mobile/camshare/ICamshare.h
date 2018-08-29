//
//  ICamshare.h
//  CamshareFramework
//
//  Created by Max on 2017/3/2.
//  Copyright © 2017年 alex shum. All rights reserved.
//

#ifndef ICamshare_h
#define ICamshare_h

namespace camshare {
    typedef enum UserType {
        UserType_Lady = 0,
        UserType_Man = 1,
        UserType_Count,
    } UserType;
    
    // 采集视频的方向
    typedef enum Direction {
        Direction_0 = 0,
        Direction_90 = 90,
        Direction_180 = 180,
        Direction_270 = 270,
    } Direction;
    
    // 采集视频的方向
    typedef enum DeviceType {
        DeviceType_Android = 0,
        DeviceType_IOS = 1,
        DeviceType_Other = 2,
    } DeviceType;
    
    // 原始视频数据格式
    typedef enum VIDEO_FORMATE_TYPE{
        VIDEO_FORMATE_NONE    = 0,
        VIDEO_FORMATE_NV21    = 1,
        VIDEO_FORMATE_NV12    = 2,
        VIDEO_FORMATE_YUV420P = 3,
        VIDEO_FORMATE_RGB565  = 4,
        VIDEO_FORMATE_RGB24   = 5
    }VIDEO_FORMATE_TYPE;
    
class ICamshare {
public:
    virtual ~ICamshare(){};
    
};
}
#endif /* ICamshare_h */
