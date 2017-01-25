/*
 * CaptureData.h
 *
 *  Created on: 2016年12月16日
 *      Author: Alex
 */

#ifndef CAPTUREBUFFER_H_
#define CAPTUREBUFFER_H_

#include "Rtp2H264VideoTransfer.h"
#include "IAutoLock.h"
#include <list>
#include <string>
using namespace std;

typedef enum {
	VIDEO_FORMATE_NV21 = 17
}VIDEO_FORMATE_TYPE;

typedef struct CaptureBuffer {
	CaptureBuffer() {
		data = NULL;
		len = 0;
		timeStamp = 0;
		this->timeInterval = 0;
		width = 0;
		heigth = 0;
		direction = 0;
	}
	CaptureBuffer(const CaptureBuffer* item) {
		this->data = NULL;
		this->len = 0;
		this->timeStamp = 0;
		this->timeInterval = 0;
		this->width = 0;
		this->heigth = 0;
		this->direction = 0;
		if( item && item->data != NULL ) {
			this->data = new unsigned char[item->len + 1];
			this->len = item->len;
			this->timeStamp = item->timeStamp;
			memcpy(this->data, item->data, item->len);
			this->data[len] = '\0';
			this->width = item->width;
			this->heigth = item->heigth;
			this->direction = item->direction;
			this->timeInterval = item->timeInterval;
		}
	}
	~CaptureBuffer() {
		if( data ) {
			delete[] data;
			data = NULL;
		}
	}
	CaptureBuffer(unsigned char* data, unsigned int len, uint64_t timeStamp, unsigned int width, unsigned int heigth, unsigned int direction) {
		this->data = new unsigned char[len + 1];
		this->len = len;
		this->timeStamp = timeStamp;
		this->timeInterval = 0;
		memcpy(this->data, data, len);
		this->data[len] = '\0';
		this->width = width;
		this->heigth = heigth;
		this->direction = direction;
	}

	unsigned char* data;
	unsigned int len;
	uint64_t timeStamp;
	unsigned int timeInterval;
	unsigned int width;
	unsigned int heigth;
	unsigned int direction;

}CaptureBuffer;

class CaptureData {
public:
	CaptureData();
	~CaptureData();
public:
	// 视频数据是否是空的
	bool IsVideoDataEmpty();
	// 获取视频数据个数
	int GetVideoDataSize();
    // 获取一个视频数据
	CaptureBuffer* PopVideoData();
	// 插入一个视频数据
	bool PushVideoData(CaptureBuffer* item);
	// 清空视频队列和设置插入标志
	void CleanDataList(bool isStart);
	// 根据帧率得到一帧时间
	void SetCaptureFrameTime(int rate);
	// 设置发送视频的宽高
	void SetSendVideoSize(int width, int height);
	// 清空视频队列
	void CleanVideoDataList();
	// 选择视频采集格式
	int ChooseVideoFormate(int* videoFormate, int size, int deviceType);
private:
	// 整理队列符合采集数据
	//void HandleAndSortVideoData(CaptureBuffer* item);
	// 清空视频队列
	//void CleanVideoDataList();

private:
//	// 没有处理的采集视频数据队列
	list<CaptureBuffer*> CaptureDataList;
	//采集的数据
	//CaptureBuffer* mCaptureData;
	IAutoLock* mCaptureDataLock;
	// 没有处理的采集视频数据队列最小的个数
	int minListLen;
	// 没有处理的采集视频数据队列最大的个数
	int MaxListLen;
	// 上个编码的视频数据 （只有一个）
	CaptureBuffer* mLastEncodeData;
	// 视频采集一帧的时间（毫秒）
	int mCaptureFrameMS;
	// 发送的视频宽
	int mWidth;
	// 发送的视频高
	int mHeight;
	// 视频宽因数
	int mWFactor;
	// 视频高因数
	int mHFactor;

	// 开始插入数据标记
	bool mIsPush;

};

#endif /* CAPTUREBUFFER_H_ */
