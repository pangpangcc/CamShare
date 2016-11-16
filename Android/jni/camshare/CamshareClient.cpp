/*
 * CamshareClient.cpp
 *
 *  Created on: 2016年10月28日
 *      Author: max
 */

#include <CamshareClient.h>

#include <common/KLog.h>

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
			}

			FileLog("CamshareClient",
					"ClientRunnable::onRun( "
					"[Disconnect] "
					")"
					);

		} else {
			// 连接失败
			mContainer->mRtmpClient.Close();
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

	mpRunnable = new ClientRunnable(this);
	mRtmpClient.SetRtmpClientListener(this);
}

CamshareClient::~CamshareClient() {
	// TODO Auto-generated destructor stub
	Stop();

	if( mpRunnable ) {
		delete mpRunnable;
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

	if( IsRunning() ) {
		Stop();
	}

	mbRunning = true;

	// Create decoder
	bFlag = mH264Decoder.Create();

	this->hostName = hostName;
	this->user = user;
	this->password = password;
	this->site = site;
	this->sid = sid;
	this->userType = userType;

	// Create rtmp client thread
	mClientThread.start(mpRunnable);

	FileLog("CamshareClient",
			"CamshareClient::Start( "
			"this : %p, "
			"bFlag : %s "
			")",
			this,
			bFlag?"true":"false"
			);

	if( !bFlag ) {
		Stop();
	}

	return bFlag;
}

void CamshareClient::Stop() {
	FileLog("CamshareClient",
			"CamshareClient::Stop( "
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

	mbRunning = false;

	mRtmpClient.Shutdown();
	mClientThread.stop();
	mH264Decoder.Destroy();
	mRtmpClient.Close();
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
	mH264Decoder.SetRecordFile(filePath);
}

void CamshareClient::OnConnect(RtmpClient* client, const string& sessionId) {
	char custom[1024];
	sprintf(custom, "sid=%s&userType=%d", sid.c_str(), userType);
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
			"bSuccess : %s "
			")",
			this,
			success?"true":"false"
			);

	if( success ) {
		if( client->GetUser() == client->GetDest() ) {
			// 进入自己会议室, 上传
			client->CreatePublishStream();

		} else {
			// 进入别人的会议室, 下载
			client->CreateReceiveStream();
			// 设置录制文件
			char filePath[1024];
			snprintf(filePath, sizeof(filePath), "/sdcard/camshare/%s.h264", this->dest.c_str());
			mH264Decoder.SetRecordFile(filePath);
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
//	FileLog("CamshareClient",
//			"CamshareClient::OnRecvVideo( "
//			"this : %p, "
//			"size : %u "
//			")",
//			this,
//			size
//			);

	char* frame = new char[1024 * 1024];
	int frameLen = 0, width = 0, height = 0;
	bool bFlag = mH264Decoder.DecodeFrame(data, size, frame, frameLen, width, height);
	if( bFlag && mpCamshareClientListener != NULL ) {
		if( mFrameWidth != width || mFrameHeight != height ) {
			mFrameWidth = width;
			mFrameHeight = height;
			mpCamshareClientListener->OnRecvVideoSizeChange(this, mFrameWidth, mFrameHeight);
		}

		mpCamshareClientListener->OnRecvVideo(this, frame, frameLen, timestamp);
	}
	delete[] frame;
}
