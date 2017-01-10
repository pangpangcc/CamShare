/*
 * CamshareClient.cpp
 *
 *  Created on: 2016年10月28日
 *      Author: max
 */

#include <CamshareClient.h>

#include <common/KLog.h>
#include <common/CommonFunc.h>

// 关键帧的位置
typedef struct KeyFramePosition {
	KeyFramePosition() {
		pspStartPosition = 0;    // psp开始位置
		pspLen = 0;              // psp长度
		ppsStartPosition = 0;    // pps开始位置
		ppsLen = 0;              // pps长度
		iFrameStartPosition = 0; // i帧的开始位置
		iFrameLen = 0;
	}

	~KeyFramePosition() {
	}

	int pspStartPosition;    // psp开始位置
	int pspLen;              // psp长度
	int ppsStartPosition;    // pps开始位置
	int ppsLen;              // pps长度
	int iFrameStartPosition; // i帧的开始位置
	int iFrameLen;           // i帧的长度
}KeyFramePosition;


class ClientRunnable : public KRunnable {
public:
	ClientRunnable(CamshareClient *container) {
		mContainer = container;
	}
	virtual ~ClientRunnable() {
		mContainer = NULL;
	}
protected:
	void onRun() {
		if( mContainer->mRtmpClient.Connect(mContainer->hostName) ) {
			RtmpPacket recvPacket;
			while( mContainer->mRtmpClient.RecvRtmpPacket(&recvPacket) ) {

				RTMP_PACKET_TYPE type = mContainer->mRtmpClient.ParseRtmpPacket(&recvPacket);
				recvPacket.FreeBody();
				recvPacket.Reset();
			}

			FileLog("CamshareClient",
					"ClientRunnable::onRun( "
					"[Disconnect] "
					")"
					);

		} else {
			// 连接失败
			if( mContainer->mpCamshareClientListener ) {
				mContainer->mpCamshareClientListener->OnDisconnect(mContainer);
			}
		}
	}

private:
	CamshareClient *mContainer;
};

CamshareClient::CamshareClient() {
	// TODO Auto-generated constructor stub
	mpCamshareClientListener = NULL;
	mbRunning = false;

	hostName = "";
	user = "";
	password = "";
	site = "";
	sid = "";
	userType = -1;

	dest = "";

	mFrameWidth = 0;
	mFrameHeight = 0;

	mFilePath = "";

	mpRunnable = new ClientRunnable(this);
	mRtmpClient.SetRtmpClientListener(this);

	mVideoData = new CaptureData;
	mVideoDataThread = NULL;
	mVideoDataThreadStart = false;
	mCaptureVideoStart    = false;
	mEncodeTimeInterval   = 1000/6;
}

CamshareClient::~CamshareClient() {
	// TODO Auto-generated destructor stub
	Stop(true);

	if( mpRunnable ) {
		delete mpRunnable;
	}

	if (mVideoData){
		delete mVideoData;
		mVideoData = NULL;
	}
}

bool CamshareClient::Start(
		const string& hostName,
		const string& user,
		const string& password,
		const string& site,
		const string& sid,
		UserType userType
		) {
	bool bFlag = false;

	FileLog("CamshareClient",
			"CamshareClient::Start( "
			"this : %p, "
			"hostName : %s, "
			"user : %s, "
			"site : %s, "
			"sid : %s, "
			"userType : %d "
			")",
			this,
			hostName.c_str(),
			user.c_str(),
			site.c_str(),
			sid.c_str(),
			userType
			);

	mClientMutex.lock();
	if( mbRunning ) {
		mClientMutex.unlock();
		Stop(true);
		mClientMutex.lock();
	}

	mbRunning = true;

	// Create decoder
	bFlag = mH264Decoder.Create();

	// Create encoder
	bFlag = mH264Encoder.Create();

	bFlag = StartVideoDataThread();

	this->hostName = hostName;
	this->user = user;
	this->password = password;
	this->site = site;
	this->sid = sid;
	this->userType = userType;

	// Create rtmp client thread
	mClientThread.start(mpRunnable);

	mClientMutex.unlock();

	FileLog("CamshareClient",
			"CamshareClient::Start( "
			"this : %p, "
			"bFlag : %s "
			")",
			this,
			bFlag?"true":"false"
			);

	if( !bFlag ) {
		Stop(true);
	}

	return bFlag;
}

void CamshareClient::Stop(bool bWait) {
	FileLog("CamshareClient",
			"CamshareClient::Stop( "
			"this : %p, "
			"bWait : %s, "
			"hostName : %s, "
			"user : %s, "
			"site : %s, "
			"sid : %s, "
			"userType : %d "
			")",
			this,
			bWait?"true":"false",
			hostName.c_str(),
			user.c_str(),
			site.c_str(),
			sid.c_str(),
			userType
			);

	mClientMutex.lock();
	if( mbRunning ) {
		mbRunning = false;

		mRtmpClient.Shutdown();

		if( bWait ) {
			mClientThread.stop();
		}

		mH264Decoder.Destroy();

		StopCapture();
		StopVideoDataThread();
		mH264Encoder.Destroy();

		// 关闭socket
		mRtmpClient.Close();

		FileLog("CamshareClient",
				"CamshareClient::Stop( "
				"[Finish], "
				"this : %p, "
				"bWait : %s, "
				"hostName : %s, "
				"user : %s, "
				"site : %s, "
				"sid : %s, "
				"userType : %d "
				")",
				this,
				bWait?"true":"false",
				hostName.c_str(),
				user.c_str(),
				site.c_str(),
				sid.c_str(),
				userType
				);
	}

	mClientMutex.unlock();
}

bool CamshareClient::MakeCall(const string& dest, const string serverId) {
	FileLog("CamshareClient",
			"CamshareClient::MakeCall( "
			"this : %p, "
			"user : %s, "
			"site : %s, "
			"dest : %s, "
			"serverId : %s, "
			"userType : %d "
			")",
			this,
			user.c_str(),
			site.c_str(),
			dest.c_str(),
			serverId.c_str(),
			userType
			);
	this->dest = dest;

	char conference[1024];
	sprintf(conference, "%s|||%s|||%s", this->dest.c_str(), serverId.c_str(), site.c_str());

	return mRtmpClient.MakeCall(conference);
}

bool CamshareClient::Hangup() {
	FileLog("CamshareClient",
			"CamshareClient::Hangup( "
			"this : %p, "
			"user : %s, "
			"site : %s, "
			"dest : %s, "
			"userType : %d "
			")",
			this,
			user.c_str(),
			site.c_str(),
			dest.c_str(),
			userType
			);

	return mRtmpClient.Hangup();
}

bool CamshareClient::IsRunning() {
	return mbRunning;
}

void CamshareClient::SetCamshareClientListener(CamshareClientListener *listener) {
	mpCamshareClientListener = listener;
}

void CamshareClient::SetRecordFilePath(const string& filePath) {
	mFilePath = filePath;
	MakeDir(mFilePath);

//	// 设置录制文件
//	char Path[1024] = {'\0'};
//	snprintf(Path, sizeof(Path), "%s/rtmpt.txt", mFilePath.c_str());
//	mRtmpClient.SetRecordFile(Path);
}


// 编码和发送视频数据
bool CamshareClient::InsertVideoData(unsigned char* data, unsigned int len,  unsigned long timesp, int width, int heigh, int direction){

	FileLog("CamshareClient",
			"CamshareClient::InsertVideoData(start "
			"mVideoData:%p"
			")",
			mVideoData
			);
	bool result = false;
	if (mCaptureVideoStart && data && len > 1)
	{

		if(mVideoData)
		{
			CaptureBuffer* item = new CaptureBuffer(data, len, timesp, width, heigh, direction);
			result = mVideoData->PushVideoData(item);
		}
	}

	FileLog("CamshareClient",
			"CamshareClient::InsertVideoData(end "
			")"
			);
	return result;

}

bool CamshareClient::EncodeVideoData(CaptureBuffer* item)
{
	bool result = false;
	// 编码后的数据
	unsigned char* videoDate = new unsigned char[512 * 1024];
	// 编码后的数据的长度
	int dateLen   = 0;
	// 编码后是否是关键帧
	bool isKeyFrame = false;

	// h264编码
	result = mH264Encoder.EncodeFrame(item->data, item->len, videoDate, dateLen, isKeyFrame, item->width, item->heigth);

	if (result)
	{

//		FileLog("CamshareClient",
//				"Jni::CamshareClient1"
//				"desData[4]:%x,"
//				//"isKey_frame:%d,"
//				"dateLen:%d"
//				")",
//				//pictType,
//				//isKey_frame,
//				videoDate[4],
//				dateLen
//				);
		if (isKeyFrame){

			// 关键帧
			KeyFramePosition *position = new KeyFramePosition;
			// 折分关键帧为psp，pps和I帧
			bool result = separateKeyFrame(videoDate, dateLen, position);

			// psp
			unsigned char* sps = new unsigned char[position->pspLen];
			memcpy(sps, videoDate + position->pspStartPosition, position->pspLen);
			// 组包
			result = mRtmpClient.RtmpToPacketH264(sps, position->pspLen, item->timeInterval);

			//pps
			unsigned char* pps = new unsigned char[position->ppsLen];
			memcpy(pps, videoDate + position->ppsStartPosition, position->ppsLen);

			FileLog("CamshareClient",
					"Jni::CamshareClient1"
					"dateLen:%d,"
					"pspLen:%d,"
					"ppsLen:%d"
					")",
					dateLen,
					position->pspLen,
					position->ppsLen

					);

			// 组包（注意psp和pps才组成头）
			result = mRtmpClient.RtmpToPacketH264(pps, position->ppsLen, item->timeInterval);

			unsigned char* iFrame = new unsigned char[position->iFrameLen];
			// I帧
			memcpy(iFrame, videoDate + position->iFrameStartPosition, position->iFrameLen);
			result = mRtmpClient.RtmpToPacketH264(iFrame, position->iFrameLen, item->timeInterval);
			delete sps;
			sps = NULL;
			delete pps;
			pps = NULL;
			delete iFrame;
			iFrame = NULL;
			delete position;
		}
		else{
			FileLog("CamshareClient",
					"Jni::CamshareClient1"
					")"
					);
			// 非I帧（这里是p帧，以后可能要）
			int startPosition = 0;
			if(videoDate[2] == 0x01 && (videoDate[3] &0x1f == 1)){
				startPosition = 3;
			}
			else if(videoDate[3] == 0x01 && ((videoDate[4] &0x1f)== 1)){
				startPosition = 4;
			}
			else
			{
				startPosition = 0;

			}

			result = mRtmpClient.RtmpToPacketH264(videoDate + startPosition, dateLen-startPosition, item->timeInterval);

		}

		//mRtmpClient.HandleSendH264();

	}


	delete(videoDate);
	return result;
}

// 发送视频数据
void CamshareClient::SendVideoData()
{
	mRtmpClient.HandleSendH264();
}

// 设置发送视频的高宽
bool CamshareClient::SetVideoSize(int width, int heigth)
{
	bool result = false;
	result = mH264Encoder.SetVideoSize(width, heigth);
	if (mVideoData)
	{
		mVideoData->SetSendVideoSize(width, heigth);
	}
	return result;
}

// 设置采集视频的采集率
bool CamshareClient::setVideRate(int rate)
{
	bool result = false;
	result = mH264Encoder.SetCaptureVideoFramerate(rate);
	if (mVideoData)
	{
		mVideoData->SetCaptureFrameTime(rate);
	}
	mEncodeTimeInterval = 1000/rate;
	return result;
}

// 开始采集
void CamshareClient::StartCapture()
{
	if(mVideoData){
		mCaptureVideoStart = true;
	}
}

// 停止采集
void CamshareClient::StopCapture()
{
	if(mVideoData){
		mCaptureVideoStart = false;
		mVideoData->CleanVideoDataList();
	}
}

void CamshareClient::OnConnect(RtmpClient* client, const string& sessionId) {
	char custom[1024];
	sprintf(custom, "sid=%s&userType=%d", sid.c_str(), userType);

	FileLog("CamshareClient",
			"CamshareClient::Login( "
			"client : %p, "
			"user : %s, "
			"site : %s, "
			"userType : %d "
			")",
			client,
			user.c_str(),
			site.c_str(),
			userType
			);

	client->Login(user, password, site, custom);
}

void CamshareClient::OnDisconnect(RtmpClient* client) {
	if( mpCamshareClientListener != NULL ) {
		mpCamshareClientListener->OnDisconnect(this);
	}
}

void CamshareClient::OnLogin(RtmpClient* client, bool success) {
	if( mpCamshareClientListener != NULL ) {
		mpCamshareClientListener->OnLogin(this, success);
	}
}

void CamshareClient::OnMakeCall(RtmpClient* client, bool success, const string& channelId) {
	FileLog("CamshareClient",
			"CamshareClient::OnMakeCall( "
			"this : %p, "
			"bSuccess : %s, "
			"user : %s, "
			"dest : %s "
			")",
			this,
			success?"true":"false",
			client->GetUser().c_str(),
			this->dest.c_str()
			);

	if( success ) {
		if( client->GetUser() == this->dest ) {			// 进入自己会议室, 上传
			client->CreatePublishStream();

			// 设置录制文件
			char filePath[1024] = {'\0'};
			char pTimeBuffer[1024] = {'\0'};

			timeval tNow;
			gettimeofday(&tNow, NULL);
			time_t stm = time(NULL);
			struct tm tTime;
			localtime_r(&stm, &tTime);
			snprintf(pTimeBuffer, 64, "%d%02d%02d_%02d%02d%02d",
					tTime.tm_year + 1900, tTime.tm_mon + 1, tTime.tm_mday, tTime.tm_hour, tTime.tm_min, tTime.tm_sec);

			snprintf(filePath, sizeof(filePath), "%s/%s_%s_%s.h264", mFilePath.c_str(), this->user.c_str(), this->dest.c_str(), pTimeBuffer);
			mH264Encoder.SetRecordFile(filePath);
			// 开始采集
			StartCapture();

		} else {
			// 进入别人的会议室, 下载
			client->CreateReceiveStream();
			// 设置录制文件
			char filePath[1024] = {'\0'};
			char pTimeBuffer[1024] = {'\0'};

			timeval tNow;
			gettimeofday(&tNow, NULL);
			time_t stm = time(NULL);
			struct tm tTime;
			localtime_r(&stm, &tTime);
			snprintf(pTimeBuffer, 64, "%d%02d%02d_%02d%02d%02d",
					tTime.tm_year + 1900, tTime.tm_mon + 1, tTime.tm_mday, tTime.tm_hour, tTime.tm_min, tTime.tm_sec);

			//snprintf(filePath, sizeof(filePath), "%s/%s_%s_%s.h264", mFilePath.c_str(), this->user.c_str(), this->dest.c_str(), pTimeBuffer);
			//mH264Decoder.SetRecordFile(filePath);

			//snprintf(filePath, sizeof(filePath), "%s/rtmpt.txt", mFilePath.c_str());
			//mRtmpClient.SetRecordFile(filePath);

			StopCapture();
		}

	}

	if( mpCamshareClientListener != NULL ) {
		mpCamshareClientListener->OnMakeCall(this, success);
	}
}

void CamshareClient::OnHangup(RtmpClient* client, const string& channelId, const string& cause) {
	if( mpCamshareClientListener != NULL ) {
		mpCamshareClientListener->OnHangup(this, cause);
	}
}

void CamshareClient::OnCreatePublishStream(RtmpClient* client) {

}

void CamshareClient::OnHeartBeat(RtmpClient* client) {

}

void CamshareClient::OnRecvAudio(RtmpClient* client, const char* data, unsigned int size) {
//	FileLog("CamshareClient",
//			"CamshareClient::OnRecvAudio( "
//			"this : %p, "
//			"size : %u "
//			")",
//			this,
//			size
//			);
}

void CamshareClient::OnRecvVideo(RtmpClient* client, const char* data, unsigned int size, unsigned int timestamp) {
	FileLog("CamshareClient",
			"CamshareClient::OnRecvVideo( start"
			"this : %p, "
			"size : %u "
			")",
			this,
			size
			);
	static uint64_t Newtime = GetCurrentTime();
	uint64_t InvTime = GetCurrentTime() - Newtime;
	Newtime = GetCurrentTime();

	char* frame = new char[512 * 1024];
	int frameLen = 0, width = 0, height = 0;
	bool bFlag = mH264Decoder.DecodeFrame(data, size, frame, frameLen, width, height);
	if( bFlag && mpCamshareClientListener != NULL ) {
		if( mFrameWidth != width || mFrameHeight != height ) {
			mFrameWidth = width;
			mFrameHeight = height;
			mpCamshareClientListener->OnRecvVideoSizeChange(this, mFrameWidth, mFrameHeight);
		}

			FileLog("CamshareClient",
					"CamshareClient::OnRecvVideo( end"
					"this : %p, "
					"frameLen : %d "
					"timestamp : %d "
					")",
					this,
					frameLen,
					timestamp
					);
		mpCamshareClientListener->OnRecvVideo(this, frame, frameLen, timestamp);
	}


	delete[] frame;

}

// 分离关键帧（psp，pps和I帧）
bool CamshareClient::separateKeyFrame(unsigned char* keyFrame, int len, KeyFramePosition* position)
{
	bool rusult = true;
	int startPosition = 0;//开始位置
	int endPosition = 0;// 结束位置
	//判断psp的开始位置
	if(keyFrame[2] == 0x01 && (keyFrame[3] &0x1f == 7)){
		startPosition = 3;
	}
	else if(keyFrame[3] == 0x01 && ((keyFrame[4] &0x1f)== 7)){
		startPosition = 4;
	}
	else
	{
		FileLog("CamshareClient",
				"CamshareClient::separateKeyFrame( "
				"keyFrame[2]:%x,keyFrame[3]:%x,keyFrame[4]:%x"
				"this is no psp and pps frame)",
				keyFrame[2], keyFrame[3], keyFrame[4]
				);
		return false;
	}
	// 得到psp的开始位置
	position->pspStartPosition = startPosition;

	// 遍历关键帧
	for(int i = startPosition + 1; i + 1 < len; i++)
	{
		// 判断是否是0；判断一帧的标识是 0x00 0x00 0x01 或者  0x00 0x00 0x00 0x01，而后一位判断nal的类型是什么
		if(keyFrame[i])
			continue;
		if(i > 0 && keyFrame[i + 1] == 0 && keyFrame[i+2] < 3){
			if(keyFrame[i+2] == 0x01 && ((keyFrame[i+3] & 0x1f) == 8)){
				// pps
				endPosition = i - 1;
				position->pspLen = endPosition - startPosition  +1;
				startPosition = i + 3;
				i = startPosition + 1;
				position->ppsStartPosition =  startPosition;
			}
			else if(keyFrame[i+3] == 0x01 && ((keyFrame[i+4] & 0x1f) == 8))
			{
				// pps
				endPosition = i - 1;
				position->pspLen = endPosition - startPosition + 1;
				startPosition = i + 4;
				i = startPosition + 1;
				position->ppsStartPosition =  startPosition;
			}
			else if(keyFrame[i+2] == 0x01 && ((keyFrame[i+3] & 0x1f) == 5))
			{
				// I帧
				endPosition = i - 1;
				position->ppsLen = endPosition - startPosition + 1;
				startPosition = i + 3;
				i =  startPosition + 1;
				position->iFrameStartPosition = startPosition;
				position->iFrameLen = len - startPosition - 1;
				break;
			}
			else if(keyFrame[i+3] == 0x01 && ((keyFrame[i+4] & 0x1f) == 5))
			{
				// I帧
				endPosition = i - 1;
				position->ppsLen = endPosition - startPosition + 1;
				startPosition = i + 4;
				i =  startPosition + 1;
				position->iFrameStartPosition = startPosition;
				position->iFrameLen = len - startPosition;
				break;
			}

		}
	}
	return rusult;
}

// ------- 视频数据线程----------------
bool CamshareClient::StartVideoDataThread()
{
	FileLog("CamshareClient",
			"CamshareClient::StartVideoDataThread(start"
			")"
			);
	bool result = false;

	// 停止之前视频数据线程
	StopVideoDataThread();

	mVideoDataThread = IThreadHandler::CreateThreadHandler();

	if (NULL != mVideoDataThread)
	{
		mVideoDataThreadStart = true;
		result = mVideoDataThread->Start(CamshareClient::VideoDataThread, this);
		if (!result){
			mVideoDataThreadStart = false;
		}
	}
	FileLog("CamshareClient",
			"CamshareClient::StartVideoDataThread(end"
			")"
			);
	return result;
}

// 视频数据线程结束
void CamshareClient::StopVideoDataThread()
{
	FileLog("CamshareClient",
			"CamshareClient::StopVideoDataThread(start"
			")"
			);
	if (NULL != mVideoDataThread)
	{
		mVideoDataThreadStart = false;
		mVideoDataThread->WaitAndStop();
		IThreadHandler::ReleaseThreadHandler(mVideoDataThread);
		mVideoDataThread = NULL;
	}
	FileLog("CamshareClient",
			"CamshareClient::StopVideoDataThread(end"
		    ")"
			);
}

// 视频数据线程函数
TH_RETURN_PARAM CamshareClient::VideoDataThread(void* obj){
	CamshareClient* pThis = (CamshareClient*)obj;
	pThis->VideoDataThreadProc();
	return 0;
}

// 视频数据线程处理函数
void CamshareClient::VideoDataThreadProc()
{
	while (mVideoDataThreadStart)
	{

		bool bFlag = false;
		if (mCaptureVideoStart && !mVideoData->IsVideoDataEmpty())
		{
			uint64_t timeInterval = GetCurrentTime() - mEncodeTimeStamp;
			if (timeInterval < mEncodeTimeInterval)
			{
				if (timeInterval < (mEncodeTimeInterval/2))
				{
					Sleep(mEncodeTimeInterval/3);
					//Sleep(5);
				}
				else if(mEncodeTimeInterval - timeInterval> 30)
				{
					Sleep(10);
				}
				else
				{
					Sleep(5);
				}
			}
			else
			{
				mEncodeTimeStamp = GetCurrentTime();
				SendVideoData();
				CaptureBuffer* item = mVideoData->PopVideoData();
				FileLog("CamshareClient",
						"CamshareClient::VideoDataThreadProc(start"
						"timeInterval:%d"
						")",
						timeInterval
						);
				if(NULL != item){
					bFlag = true;

					unsigned char* frame = new unsigned char[512 * 1024];
					//int frameLen = 0, width = 0, height = 0;
					mpCamshareClientListener->OnCallVideo(this, item->data, item->width, item->heigth, timeInterval);
					item->timeInterval = timeInterval;
					EncodeVideoData(item);
					delete frame;
					frame = NULL;
					FileLog("CamshareClient",
								"CamshareClient::VideoDataThreadProc(end"
								")"
								);
				}
				Sleep(5);
			}
		}
		else {
			Sleep(5);
		}
	}
}

// 设置编码时间
void CamshareClient::SetEncodeTime()
{
	mEncodeTimeStamp = GetCurrentTime();
}
// 设置发送时间
void CamshareClient::SetSendTime()
{
	mSendTimeStamp = GetCurrentTime();
}

// 获取当前时间
uint64_t CamshareClient::GetCurrentTime()
{
	struct timeval tv;
	gettimeofday(&tv, 0);
	return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

