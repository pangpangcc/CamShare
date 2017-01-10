/*
 * CamshareClient.h
 *
 *  Created on: 2016年10月28日
 *      Author: max
 */

#ifndef CAMSHARE_CAMSHARECLIENT_H_
#define CAMSHARE_CAMSHARECLIENT_H_

#include <string>
using namespace std;

#include <common/KThread.h>

#include <rtmp/RtmpClient.h>
#include "H264Decoder.h"
#include "H264Encoder.h"

#include <rtmp/IThreadHandler.h>
#include "CaptureData.h"

typedef enum UserType {
	UserType_Man = 0,
	UserType_Lady,
	UserType_Count,
}UserType;

struct KeyFramePosition;
class CamshareClient;
class CamshareClientListener {
public:
	virtual ~CamshareClientListener(){};

	virtual void OnLogin(CamshareClient *client, bool success) = 0;
	virtual void OnMakeCall(CamshareClient *client, bool success) = 0;
	virtual void OnHangup(CamshareClient *client, const string& cause) = 0;
	virtual void OnDisconnect(CamshareClient* client) = 0;
	virtual void OnRecvVideo(CamshareClient* client, const char* data, unsigned int size, unsigned int timestamp) = 0;
	virtual void OnRecvVideoSizeChange(CamshareClient* client, unsigned int width, unsigned int height) = 0;
	virtual void OnCallVideo(CamshareClient* client, const char* data, unsigned int width, unsigned int heigth, unsigned int time) = 0;
};

class ClientRunnable;
class CamshareClient : public RtmpClientListener {
	friend class ClientRunnable;

public:
	CamshareClient();
	virtual ~CamshareClient();

	/**
	 * 开始连接并登录流媒体服务器
	 * @param	hostName	服务器地址	example:127.0.0.1
	 * @param	user		用户名		example:CM123456
	 * @param	password	密码			example:123456
	 * @param	site		站点Id		example:1
	 * @param	sid			php登录参数	example:SESSION123456
	 * @param	userType	php登录参数	example:UserType.UserType_Man
	 * @return	成功/失败
	 */
	bool Start(
			const string& hostName,
			const string& user,
			const string& password,
			const string& site,
			const string& sid,
			UserType userType
			);

	/**
	 * 拨号进入会议室
	 * @param	dest		会议室名字	example:P123456
	 * @param	serverId	女士服务器Id	example:PC1
	 * @return	成功/失败
	 */
	bool MakeCall(
			const string& dest,
			const string serverId
			);

	/**
	 *	退出当前会议室
	 *	@return	成功/失败
	 */
	bool Hangup();

	/**
	 * 停止连接或者断开流媒体服务器
	 */
	void Stop(bool bWait = false);

	/**
	 * 是否正在运行
	 */
	bool IsRunning();

	/**
	 * 设置回调
	 */
	void SetCamshareClientListener(CamshareClientListener *listener);

	/**
	 * 设置录制文件夹
	 */
	void SetRecordFilePath(const string& filePath);

	// 编码和发送视频数据
	bool InsertVideoData(unsigned char* data, unsigned int len, unsigned long timesp, int width, int heigh,  int direction);
    // 编码视频数据
	bool EncodeVideoData(CaptureBuffer* item);
    // 发送视频数据
	void SendVideoData();
	// 设置采集视频的高宽
	bool SetVideoSize(int width, int heigth);
	// 设置采集视频的方向
	bool SetVideoDirection(int direction);
	// 设置采集视频的采集率
	bool setVideRate(int rate);
	// 开始采集
	void StartCapture();
	// 停止采集
	void StopCapture();

public:
	void OnConnect(RtmpClient* client, const string& sessionId);
	void OnDisconnect(RtmpClient* client);
	void OnLogin(RtmpClient* client, bool success);
	void OnMakeCall(RtmpClient* client, bool success, const string& channelId);
	void OnHangup(RtmpClient* client, const string& channelId, const string& cause);
	void OnCreatePublishStream(RtmpClient* client);
	void OnHeartBeat(RtmpClient* client);
	void OnRecvAudio(RtmpClient* client, const char* data, unsigned int size);
	void OnRecvVideo(RtmpClient* client, const char* data, unsigned int size, unsigned int timestamp);

private:
	// 分离关键帧（psp，pps和I帧）
	bool separateKeyFrame(unsigned char* keyFrame, int len, KeyFramePosition* position);
	// ------- 视频数据线程----------------
	bool StartVideoDataThread();
	// 视频数据线程结束
	void StopVideoDataThread();
	// 视频数据线程函数
	static TH_RETURN_PARAM VideoDataThread(void* obj);
	// 视频数据线程处理函数
	void VideoDataThreadProc();
	// 设置编码时间
	void SetEncodeTime();
	// 设置发送时间
	void SetSendTime();
	// 获取当前时间
	uint64_t GetCurrentTime();

private:
	RtmpClient mRtmpClient;
	ClientRunnable* mpRunnable;
	KThread mClientThread;
	H264Decoder mH264Decoder;
	// 编码器
	H264Encoder mH264Encoder;
    CaptureData *mVideoData;
	// 获取视频数据线程
	IThreadHandler* mVideoDataThread;
	// 获取视频数据线程启动标记
	bool mVideoDataThreadStart;
	// 采集视频开启标记
	bool mCaptureVideoStart;
	// 编码间隔时间
	int mEncodeTimeInterval;
	// 编码时间
	uint64_t mEncodeTimeStamp;
	// 发送时间
	uint64_t mSendTimeStamp;


	CamshareClientListener* mpCamshareClientListener;
	bool mbRunning;

	string hostName;
	string user;
	string password;
	string site;
	string sid;
	int userType;

	string dest;

	unsigned int mFrameWidth;
	unsigned int mFrameHeight;

	string mFilePath;

	/**
	 * 状态锁
	 */
	KMutex mClientMutex;
};

#endif /* CAMSHARE_CAMSHARECLIENT_H_ */
