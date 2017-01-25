/*
 * H264Encoder.cpp
 *
 *  Created on: 2016年12月16日
 *      Author: alex
 */

#include <CaptureData.h>
#include <common/KLog.h>

#include <common/Arithmetic.h>


// 旋转0度，裁剪，NV21转YUV420P
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

//旋转0度，裁剪，NV21转YUV420P（竖着录制后摄像头）
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

//旋转0度，裁剪，NV21转YUV420P
static unsigned char* detailPic180(unsigned char* data, int imageWidth, int imageHeight, int newImageW, int newImageH) {
	unsigned char* yuv_temp =new unsigned char[newImageW*newImageH*3/2];
	int deleteW = (imageWidth - newImageW) / 2;
	int deleteH = (imageHeight - newImageH) / 2;
	//处理y 旋转加裁剪
	int i, j;
	int index = newImageW * newImageH;
	for (j = deleteH; j < imageHeight- deleteH; j++) {
		for (i = deleteW; i < imageWidth- deleteW; i++)
			yuv_temp[--index]= data[j * imageWidth + i];
	}

	//处理u
	index= newImageW * newImageH * 5 / 4;

	for (i = imageHeight + deleteH / 2;i < imageHeight / 2 * 3 - deleteH / 2; i++)
		for (j = deleteW + 1; j< imageWidth - deleteW; j += 2)
			yuv_temp[--index]= data[i * imageWidth + j];

	//处理v 旋转裁剪加格式转换
	index= newImageW * newImageH * 3 / 2;
	for (i = imageHeight + deleteH / 2;i < imageHeight / 2 * 3 - deleteH / 2; i++)
		for (j = deleteW; j < imageWidth- deleteW; j += 2)
			yuv_temp[--index]= data[i * imageWidth + j];

	return yuv_temp;
}

//旋转0度，裁剪，NV21转YUV420P（竖着录制前摄像头）
static unsigned char* detailPic270(unsigned char* data, int imageWidth, int imageHeight, int newImageW, int newImageH) {
	unsigned char* yuv_temp =new unsigned char[newImageW*newImageH*3/2];
    int deleteW = (imageWidth - newImageH) / 2;
    int deleteH = (imageHeight - newImageW) / 2;
    int i, j;
    //处理y 旋转加裁剪
    for (i = 0; i < newImageH; i++){
        for (j = 0; j < newImageW; j++){
            yuv_temp[i* newImageW + j] = data[imageWidth * (deleteH + j) + imageWidth - deleteW - i];
        }
    }

    //处理u 旋转裁剪加格式转换
    int index = newImageW * newImageH;
    for (i = imageWidth - deleteW - 1;i > deleteW; i -= 2)
        for (j = imageHeight + deleteH / 2;j < imageHeight / 2 * 3 - deleteH / 2; j++)
            yuv_temp[index++]= data[(j) * imageWidth + i];

    //处理v 旋转裁剪加格式转换

    for (i = imageWidth - deleteW - 2;i >= deleteW; i -= 2)
        for (j = imageHeight + deleteH / 2;j < imageHeight / 2 * 3 - deleteH / 2; j++)
            yuv_temp[index++]= data[(j) * imageWidth + i];

    return yuv_temp;
}



CaptureData::CaptureData() {
	list<CaptureBuffer*> CaptureDataList;
	mCaptureDataLock = IAutoLock::CreateAutoLock();
	mCaptureDataLock->Init();
	minListLen = 1;
	MaxListLen = 3;
	mLastEncodeData = NULL;
	// 默认一帧的时间
	mCaptureFrameMS = 1000/6;
//
//	// 发送的视频宽
//	mWidth = 176;
//	// 发送的视频高
//	mHeight = 144;
//
	SetSendVideoSize(176, 144);

	mIsPush = false;
}

CaptureData::~CaptureData() {
	// TODO Auto-generated destructor stub

	CleanDataList(false);
	delete mLastEncodeData;
	mLastEncodeData = NULL;
	IAutoLock::ReleaseAutoLock(mCaptureDataLock);
}

// 视频数据是否是空的
bool CaptureData::IsVideoDataEmpty()
{
	bool result = false;
	if (NULL != mCaptureDataLock)
	{
		mCaptureDataLock->Lock();
	}
	result = CaptureDataList.empty();

	if (NULL != mCaptureDataLock)
	{
		mCaptureDataLock->Unlock();
	}
	return result;
}

// 获取视频数据个数
int CaptureData::GetVideoDataSize(){
	if (NULL != mCaptureDataLock)
	{
		mCaptureDataLock->Lock();
	}
	int dataSize = CaptureDataList.size();
	if (NULL != mCaptureDataLock)
	{
		mCaptureDataLock->Unlock();
	}
	return dataSize;
}

// 获取一个视频数据
CaptureBuffer* CaptureData::PopVideoData()
{
	FileLog("CamshareClient",
			 "CaptureData::PopVideoData(start"
				")"
				);
	if (NULL != mCaptureDataLock)
	{
		mCaptureDataLock->Lock();
	}
	CaptureBuffer* item = NULL;
	if (!CaptureDataList.empty()){
		item = CaptureDataList.front();
		CaptureDataList.pop_front();

		int result = 0;
		int newWidth = 0;
		int newHeight = 0;
		unsigned char* newData = NULL;
		int factor = mWFactor > mHFactor ? mWFactor : mHFactor;

		switch(item->direction)
		{
		case 0:
		{
			result = item->width / factor;
			newWidth = mWFactor * result;
			newHeight = mHFactor * result;
			newData = detailPic0(item->data, item->width, item->heigth, newWidth, newHeight);
		}
		break;
		case 90:
		{
			result = item->heigth / factor;
			newWidth = mWFactor * result;
			newHeight = mHFactor * result;
			newData = detailPic90(item->data, item->width, item->heigth, newWidth, newHeight);
		}
		break;
		case 180:
		{
			result = item->width / factor;
			newWidth = mWFactor * result;
			newHeight = mHFactor * result;
			newData = detailPic180(item->data, item->width, item->heigth, newWidth, newHeight);
		}
		break;
		case 270:
		{
			result = item->heigth / factor;
			newWidth = mWFactor * result;
			newHeight = mHFactor * result;
			newData = detailPic270(item->data, item->width, item->heigth, newWidth, newHeight);
		}
		break;
		default:
		{
			result = item->heigth / factor;
			newWidth = mWFactor * result;
			newHeight = mHFactor * result;
			newData = detailPic270(item->data, item->width, item->heigth, newWidth, newHeight);
		}
		break;
		}
		CaptureBuffer* dataItem = new CaptureBuffer(newData, newWidth*newHeight*3/2, item->timeStamp, newWidth, newHeight, item->direction);
		if (mLastEncodeData){
			dataItem->timeInterval = dataItem->timeStamp - mLastEncodeData->timeStamp;
			delete mLastEncodeData;
			mLastEncodeData = NULL;
		}
		else
		{
			dataItem->timeInterval = 0;

		}
		mLastEncodeData = new CaptureBuffer(dataItem);
		delete dataItem;
		dataItem = NULL;
		if (newData){
			delete[] newData;
			newData = NULL;
		}
	}

	if (NULL != mCaptureDataLock)
	{
		mCaptureDataLock->Unlock();
	}

	FileLog("CamshareClient",
			 "CaptureData::PopVideoData(end"
				")"
				);
	if (item)
	{
		delete item;
		item = NULL;
		return mLastEncodeData;
	}


	return NULL;
}

// 插入一个视频数据
bool CaptureData::PushVideoData(CaptureBuffer* item)
{
	FileLog("CamshareClient",
			"CaptureData::PushVideoData(start "
			")"
			);
	bool result = true;
	if (NULL != mCaptureDataLock)
	{
		mCaptureDataLock->Lock();
	}

	if (!CaptureDataList.empty())
	{
		CleanVideoDataList();
	}
	if (mIsPush){
		CaptureDataList.push_back(item);
		FileLog("CamshareClient",
				"CaptureData::PushVideoData"
				"succse"
				")"
				);
	}


	if (NULL != mCaptureDataLock)
	{
		mCaptureDataLock->Unlock();
	}
	FileLog("CamshareClient",
			"CamshareClient::PushVideoData(end "
			")"
			);
	return result;
}

// 清空视频队列和设置插入标志
void CaptureData::CleanDataList(bool isStart)
{
	FileLog("CamshareClient",
			"CaptureData::CleanDataList(start "
			"isStart :%d"
			")",
			isStart
			);
	if (NULL != mCaptureDataLock)
	{
		mCaptureDataLock->Lock();
	}
		mIsPush = isStart;
		if (mLastEncodeData){
			delete mLastEncodeData;
			mLastEncodeData = NULL;
		}

		CleanVideoDataList();

	if (NULL != mCaptureDataLock)
	{
		mCaptureDataLock->Unlock();
	}

	FileLog("CamshareClient",
			"CaptureData::CleanDataList(end "
			")"
			);
}

// 清空视频队列
void CaptureData::CleanVideoDataList(){

	for(list<CaptureBuffer*>::const_iterator itr = CaptureDataList.begin(); itr != CaptureDataList.end(); itr++) {
		CaptureBuffer* buffer = *itr;
		delete buffer;
		buffer = NULL;
	}
	CaptureDataList.clear();

}

// 根据帧率得到一帧时间
void CaptureData::SetCaptureFrameTime(int rate)
{
	mCaptureFrameMS = 1000/rate;
}


// 设置发送视频的宽高
void CaptureData::SetSendVideoSize(int width, int height)
{
	FileLog("CamshareClient",
			"CaptureData::SetSendVideoSize(start "
			"width :%d,"
			"height :%d"
			")",
			width,
			height
			);
	// 发送的视频宽
	 mWidth = width;
	// 发送的视频高
	 mHeight = height;

	 int m = width;
	 int n = height;
	 //获取最大公因数, 最后m为最大公因数
	 while(n!=0){
		 int r = m%n;
		 m = n;
		 n = r;
	 }

	 int wM = mWidth/m;
	 int hM = mHeight/m;

	 if (wM %2 || hM%2){
		 wM = wM * 2;
		 hM = hM * 2;
	 }
	 mWFactor = wM;
	 mHFactor = hM;

		FileLog("CamshareClient",
				"CaptureData::SetSendVideoSize(end "
				"mWidth :%d,"
				"mHeight :%d,"
				"mWFactor :%d,"
				"mHFactor :%d"
				")",
				mWidth,
				mHeight,
				mWFactor,
				mHFactor
				);

}


// 选择视频采集格式
int CaptureData::ChooseVideoFormate(int* videoFormate, int size, int deviceType)
{
	int result = -1;
	for (int i = 0; i < size; i++){
		if (videoFormate[i] == VIDEO_FORMATE_NV21){
			result = videoFormate[i];
			break;
		}
	}
	return result;
}
