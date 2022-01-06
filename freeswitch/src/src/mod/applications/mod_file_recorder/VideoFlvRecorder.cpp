/*
 * VideoFlvRecorder.cpp
 *
 *  Created on: 2018-08-29
 *      Author: Samson.Fan
 * Description: 录制flv视频文件及监控截图
 */

#include "VideoFlvRecorder.h"
#include "CommonFunc.h"

#include <errno.h>

#define H264_BUFFER_SIZE (128 * 1024)

VideoFlvRecorder::VideoFlvRecorder()
{
//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "mod_file_recorder: VideoFlvRecorder::VideoFlvRecorder()\n");

	ResetParam();
	mpMemoryPool = NULL;
}

VideoFlvRecorder::~VideoFlvRecorder()
{
//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "mod_file_recorder: VideoFlvRecorder::~VideoFlvRecorder()\n");

//	StopRecord();
}

// 重置参数
void VideoFlvRecorder::ResetParam()
{
	mIsRecord = false;
	mHasStartRecord = false;
	mIsVideoHandling = false;
	memset(mcVideoRecPath, 0, sizeof(mcVideoRecPath));
	memset(mcVideoDstDir, 0, sizeof(mcVideoDstDir));
	memset(mcFinishShell, 0, sizeof(mcFinishShell));

	mpFlvFile = NULL;
	mpHandle = NULL;

	mLocalTimestamp = 0;

	mpVideoQueue = NULL;
	mpFreeBufferQueue = NULL;

	mbNaluStart = SWITCH_FALSE;
	mpNaluBuffer = NULL;

	miVideoDataBufferLen = 0;
	miVideoDataBufferSize = 0;
	mpVideoDataBuffer = NULL;
	miCreateBufferCount = 0;

	mIsPicHandling = false;
	memset(mcPicRecPath, 0, sizeof(mcPicRecPath));
	memset(mcPicDstPath, 0, sizeof(mcPicDstPath));
	memset(mcPicShell, 0, sizeof(mcPicShell));
	miPicInterval = 0;

	mpPicMutex = NULL;
	mpPicBuffer = NULL;
	mlPicBuildTime = 0;
}

// 开始录制
bool VideoFlvRecorder::StartRecord(switch_file_handle_t *handle
		, const char *videoRecPath, const char* videoDstDir, const char* finishShell
		, const char *picRecDir, const char* picDstDir, const char* picShell, int picInterval)
{
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO
			, "mod_file_recorder: VideoFlvRecorder::StartRecord() start, recorder:%p, memory_pool:%p"
			  ", videoRecPath:%s, videoDstDir:%s, finishShell:%s"
			  ", picRecDir:%s, picDstDir:%s, picShell:%s, picInterval:%d\n"
			, this
			, (NULL!= handle ? handle->memory_pool: NULL)
			, videoRecPath, videoDstDir, finishShell
			, picRecDir, picDstDir, picShell, picInterval);

	// 确保不是正在运行，且目录没有问题
	if (!mIsRecord
		&& BuildFlvFilePath(videoRecPath)
		&& IsDirExist(videoDstDir))
	{
		bool success = true;

		strcpy(mcFinishShell, finishShell);
		strcpy(mcVideoDstDir, videoDstDir);

//		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//				, "mod_file_recorder: VideoFlvRecorder::StartRecord() mcVideoRecPath:%s, mcVideoDstDir:%s, mcFinishShell:%s\n"
//				, mcVideoRecPath, mcVideoDstDir, mcFinishShell);

		// 创建内存池
		if (success) {
			success = success
					&& (SWITCH_STATUS_SUCCESS == switch_core_new_memory_pool(&mpMemoryPool));

//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//							, "mod_file_recorder: VideoFlvRecorder::StartRecord() mpMemoryPool:%p\n"
//							, mpMemoryPool);
		}

		// 创建状态锁
		if(success) {
			success = (SWITCH_STATUS_SUCCESS
							== switch_mutex_init(&mpMutex, SWITCH_MUTEX_NESTED, mpMemoryPool));
		}


		// 记录句柄
		if (success) {
			mpHandle = handle;
		}

		// 打开文件
		if (success) {
			mpFlvFile = srs_flv_open_write(mcVideoRecPath);
			success = success && (NULL != mpFlvFile);

			if (!success) {
				// error
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
					, "mod_file_recorder: VideoFlvRecorder::StartRecord() open file fail(%d), recorder:%p, videoRecPath:%s\n"
					, errno, this, videoRecPath);
			}

//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//											, "mod_file_recorder: VideoFlvRecorder::StartRecord() mpFlvFile:%p, success:%d\n"
//											, mpFlvFile, success);
		}

		// 打开文件成功
		if(success)
		{
			// 标记为已经打开文件
			mIsRecord = true;
			mHasStartRecord = mIsRecord;

//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//							, "mod_file_recorder: VideoFlvRecorder::StartRecord() load video recorder source start\n");

			// 创建空闲buffer队列
			success = success
					&& (SWITCH_STATUS_SUCCESS
							== switch_queue_create(&mpFreeBufferQueue, SWITCH_CORE_QUEUE_LEN, mpMemoryPool));

//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//							, "mod_file_recorder: VideoFlvRecorder::StartRecord() load free buffer queue, success:%d\n"
//							, success);

			// 创建Nalu缓冲
			success = success && RenewNaluBuffer();

//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//							, "mod_file_recorder: VideoFlvRecorder::StartRecord() renew nalu buffer, success:%d\n"
//							, success);

			// 创建缓存队列
			success = success
					&& (SWITCH_STATUS_SUCCESS
							== switch_queue_create(&mpVideoQueue, SWITCH_CORE_QUEUE_LEN, mpMemoryPool));

//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//							, "mod_file_recorder: VideoFlvRecorder::StartRecord() load video queue, success:%d\n"
//							, success);

			// 创建视频处理缓存
			miVideoDataBufferLen = 0;
			miVideoDataBufferSize = H264_BUFFER_SIZE;
			success = success
					&& (NULL != (mpVideoDataBuffer = (uint8_t*)switch_core_alloc(mpMemoryPool, miVideoDataBufferSize)));


//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//							, "mod_file_recorder: VideoFlvRecorder::StartRecord() load video thread, success:%d\n"
//							, success);

			// 截图处理
			if (success)
			{
				if (picInterval > 0
					&& BuildPicRecFilePath(videoRecPath, picRecDir)
					&& BuildPicFilePath(videoRecPath, picDstDir)
					&& strlen(picShell) > 0)
				{
					// 设置参数
					miPicInterval = picInterval;
					strcpy(mcPicShell, picShell);

//					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//									, "mod_file_recorder: VideoFlvRecorder::StartRecord() load cut picture start, success:%d\n"
//									, success);

					// 创建锁
					success = success
							&& (SWITCH_STATUS_SUCCESS
									== switch_mutex_init(&mpPicMutex, SWITCH_MUTEX_NESTED, mpMemoryPool));

					// 创建buffer
					miPicDataBufferSize = H264_BUFFER_SIZE;
					success = success
							&& (NULL != (mpPicDataBuffer = (uint8_t*)switch_core_alloc(mpMemoryPool, miPicDataBufferSize)));

					success = success & (SWITCH_STATUS_SUCCESS == switch_buffer_create(mpMemoryPool, &mpSpsBuffer, 256));
					success = success & (SWITCH_STATUS_SUCCESS == switch_buffer_create(mpMemoryPool, &mpPpsBuffer, 256));
//					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//									, "mod_file_recorder: VideoFlvRecorder::StartRecord() load cut picture thread, success:%d\n"
//									, success);
				}
				else
				{
					// log
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
						, "mod_file_recorder: VideoFlvRecorder::StartRecord() param fail! recorder:%p, picInterval:%d, picRecDir:%s, picDstDir:%s"
						  ", picShell:%s, videoRecPath:%s\n"
						, this, picInterval, picRecDir, picDstDir
						, picShell, videoRecPath);

					if (!IsDirExist(picRecDir)) {
						// 目录不存在
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
							, "mod_file_recorder: VideoFlvRecorder::StartRecord() picRecDir not exist, recorder:%p, picRecDir:%s\n"
							, this, picRecDir);
					}

					if (!IsDirExist(picDstDir)) {
						// 目录不存在
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
							, "mod_file_recorder: VideoFlvRecorder::StartRecord() picDstDir not exist, recorder:%p, picDstDir:%s \n"
							, this, picDstDir);
					}
				}
			}

			// 更新打开成功标记
			mIsRecord = success;
		}
	}

	// 处理不成功
	if (!mIsRecord)
	{
		// log
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
				, "mod_file_recorder: VideoFlvRecorder::StartRecord() fail! recorder:%p, videoRecPath:%s, videoDstDir:%s, handle:%p"
				  ", mpFlvFile:%p, mpMemoryPool:%p, mpVideoQueue:%p\n"
				, this, videoRecPath, videoDstDir, handle
				, mpFlvFile, mpMemoryPool, mpVideoQueue);

		if (!IsDirExist(videoDstDir)) {
			// 目录不存在
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
				, "mod_file_recorder: VideoFlvRecorder::StartRecord() videoDstDir not exist, recorder:%p, videoDstDir:%s\n"
				, this, videoDstDir);
		}

		// 释放资源
		StopRecord();
	}

	// 打log
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO
			, "mod_file_recorder: VideoFlvRecorder::StartRecord() end, recorder:%p, mIsRecord:%d, videoRecPath:%s\n"
			, this, mIsRecord, videoRecPath);

	return mIsRecord;
}

// 停止录制
void VideoFlvRecorder::StopRecord()
{
	// log for test
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO
			, "mod_file_recorder: VideoFlvRecorder::StopRecord() start, recorder:%p, mpMemoryPool:%p, mpFile:%p, mIsRecord:%d\n"
			, this, mpMemoryPool, mpFlvFile, mIsRecord);

	mIsRecord = false;

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO
			, "mod_file_recorder: VideoFlvRecorder::StopRecord() end, recorder:%p, mIsRecord:%d\n"
			, this, mIsRecord);
}

// 重置(包括重置参数及执行close_shell)
void VideoFlvRecorder::Reset()
{
//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//				, "mod_file_recorder: VideoFlvRecorder::Reset() start, isRecord:%d\n"
//				, mIsRecord);

	// 把剩下的数据写入文件
	PutVideoBuffer2FileProc();

	// 关闭文件
	if(NULL != mpFlvFile)
	{
		srs_flv_close(mpFlvFile);
		mpFlvFile = NULL;
	}

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoFlvRecorder::Reset() close file\n");

	// ----- 释放视频录制资源 -----
	// 释放Nalu缓冲
	if (NULL != mpNaluBuffer) {
		RecycleFreeBuffer(mpNaluBuffer);
		mpNaluBuffer = NULL;
	}
	// 释放视频录制队列
	DestroyVideoQueue();

	// ----- 释放视频截图资源 -----
	// 释放picBuffer
	if (NULL != mpPicBuffer) {
		RecycleFreeBuffer(mpPicBuffer);
		mpPicBuffer = NULL;
	}

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoFlvRecorder::Reset() destroy mpNaluBuffer\n");


	// 释放空闲buffer队列
	DestroyFreeBufferQueue();

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//				, "mod_file_recorder: VideoFlvRecorder::Reset() running close shell, hasRecord:%d, handle:%p, path:%s, mcFinishShell:%s\n"
//				, mHasStartRecord, mpHandle, mcVideoRecPath, mcFinishShell);

	// 之前状态为running，执行关闭shell
	if (mHasStartRecord)
	{
		// log for test
//		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//				, "mod_file_recorder: VideoFlvRecorder::Reset() runing close shell, handle:%p\n"
//				, mpHandle);
		RunCloseShell();
	}

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//			, "mod_file_recorder: VideoFlvRecorder::Reset() finish, handle:%p\n"
//			, mpHandle);

	// 重置参数
//	switch_mutex_lock(mpMutex);
	ResetParam();
//	switch_mutex_unlock(mpMutex);

	// 释放内存池
	if (NULL != mpMemoryPool)
	{
		switch_core_destroy_memory_pool(&mpMemoryPool);
		mpMemoryPool = NULL;
	}
}

void VideoFlvRecorder::SetCallback(IVideoRecorderCallback* callback) {
	mpCallback = callback;
}

// 录制视频frame
bool VideoFlvRecorder::RecordVideoFrame(switch_frame_t *frame)
{
	bool bFlag = true;
	uint8_t nalu_type = 0;
//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoFlvRecorder::RecordVideoFrame() start handle:%p\n"
//					, mpHandle);

	if (mIsRecord)
	{
//		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//				, "mod_file_recorder: VideoFlvRecorder::RecordVideoFrame() handle:%p, datalen:%d\n"
//				, mpHandle,	frame->datalen);

		// 解析h264包
		switch_status_t status = buffer_h264_nalu(frame, mpNaluBuffer->buffer, mbNaluStart, nalu_type);

//		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//				, "mod_file_recorder: VideoFlvRecorder::RecordVideoFrame() buffer_h264_nalu, handle:%p, mbNaluStart:%d\n"
//				, mpHandle, mbNaluStart);

		if (status == SWITCH_STATUS_RESTART)
		{
			// 等待关键帧
			switch_set_flag(frame, SFF_WAIT_KEY_FRAME);
			switch_buffer_zero(mpNaluBuffer->buffer);
			mbNaluStart = SWITCH_FALSE;
			bFlag = true;

			// log for test
//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoFlvRecorder::RecordVideoFrame() handle:%p, SWITCH_STATUS_RESTART, mcVideoRecPath:%s\n"
//					, mpHandle, mcVideoRecPath);

		}
		else if (frame->m && switch_buffer_inuse(mpNaluBuffer->buffer) > 0)
		{
			// 处理第一帧timestamp
			if (0 == mLocalTimestamp) {
				mLocalTimestamp = srs_utils_time_ms();
			}

			// 更新timestamp
			int64_t now = srs_utils_time_ms();
			mpNaluBuffer->timestamp = now - mLocalTimestamp;

			// 由于这里读出数据，因此在这里判断是否为I帧
			mpNaluBuffer->isIFrame = ((nalu_type & 0x1f) == 5);

			// 增加到缓存队列
			switch_queue_push(mpVideoQueue, mpNaluBuffer);

//			// log for test
//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoFlvRecorder::RecordVideoFrame() push queue, handle:%p, size:%d \n"
//					, mpHandle, switch_buffer_inuse(mpNaluBuffer->buffer));

			// 创建新Nalu缓冲
			RenewNaluBuffer();

			// 重置标志位
			mbNaluStart = SWITCH_FALSE;
			bFlag = true;
		}
	}
	else {
		// error
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
						, "mod_file_recorder: VideoFlvRecorder::RecordVideoFrame() stoped, recorder:%p, mIsRecord:%d\n"
						, this, mIsRecord);

		bFlag = false;
	}

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoFlvRecorder::RecordVideoFrame() end handle:%p\n"
//					, mpHandle);

	return bFlag;
}

// 是否正在视频录制
bool VideoFlvRecorder::IsRecord()
{
	return mIsRecord;
}

// 录制视频线程处理
bool VideoFlvRecorder::RecordVideoFrame2FileProc()
{
	bool result = false;
	// log for test
//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//			, "mod_file_recorder: VideoFlvRecorder::RecordVideoFrame2FileProc() start, handle:%p\n"
//			, mpHandle);

	if (mIsRecord)
	{
//		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//				, "mod_file_recorder: VideoFlvRecorder::RecordVideoFrame2FileProc() pop start\n");

		// 获取缓存视频frame
		void* pop = NULL;
		while (SWITCH_STATUS_SUCCESS == switch_queue_trypop(mpVideoQueue, &pop))
		{
			result = true;
			bool success = false;

			video_frame_t* frame = (video_frame_t*)pop;
			switch_size_t inuseSize = switch_buffer_inuse(frame->buffer);
			if (inuseSize > 0)
			{
				success = WriteVideoData2FlvFile(frame);
			}

			// log for test
//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoFlvRecorder::RecordVideoFrame2FileProc() write finish, inuseSize:%d, success:%d, mcVideoRecPath:%s\n"
//					, inuseSize, success, mcVideoRecPath);

			bool isDestroyBuffer = true;
			if (success)
			{
				// handle i-frame
				if (frame->isIFrame && mIsPicHandling)
				{
					// 更新截图buffer & buffer不释放
					RenewPicBuffer(frame);
					isDestroyBuffer = false;
				}

//				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//						, "mod_file_recorder: VideoFlvRecorder::RecordVideoFrame2FileProc() check i-frame, i-frame:%d, isDestroyBuffer:%d\n"
//						, frame->isIFrame, isDestroyBuffer);
			}

//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoFlvRecorder::RecordVideoFrame2FileProc() do destroy buffer, isDestroyBuffer:%d\n"
//					, isDestroyBuffer);

			if (isDestroyBuffer)
			{
				// 回收buffer
				RecycleFreeBuffer(frame);
			}

			// 读buffer数据失败
			if (!success) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING
						, "mod_file_recorder: VideoFlvRecorder:RecordVideoFrame2FileProc() read buffer fail, recorder:%p, inuse:%d, dataBufferLen:%d, dataBufferSize:%d, mcVideoRecPath:%s\n"
						, this, inuseSize, miVideoDataBufferLen, miVideoDataBufferSize, mcVideoRecPath);
				break;
			}

		}

//		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//				, "mod_file_recorder: VideoFlvRecorder::RecordVideoFrame2FileProc() pop end\n");
	}
	else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO
				, "mod_file_recorder: VideoFlvRecorder::RecordVideoFrame2FileProc() is stop, recorder:%p, mIsRecord:%d\n"
				, this, mIsRecord);
	}

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//			, "mod_file_recorder: VideoFlvRecorder::RecordVideoFrame2FileProc() end, handle:%p\n"
//			, mpHandle);

	return result;
}

void VideoFlvRecorder::PutVideoBuffer2FileProc()
{
	// 把剩下的数据写入文件
	void* pop = NULL;
	while (SWITCH_STATUS_SUCCESS == switch_queue_trypop(mpVideoQueue, &pop))
	{
		// 写入文件
		video_frame_t* frame = (video_frame_t*)pop;
		if (switch_buffer_inuse(frame->buffer) > 0)
		{
			WriteVideoData2FlvFile(frame);
		}

		// 回收buffer
		RecycleFreeBuffer(frame);
	}
}

// 设置视频是否正在处理
void VideoFlvRecorder::SetVideoHandling(bool isHandling)
{
//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//			, "mod_file_recorder: VideoFlvRecorder::SetVideoHandling() start recorder:%p, isHandling:%d\n"
//			, this, isHandling);

	switch_mutex_lock(mpMutex);
	mIsVideoHandling = isHandling;
	bool result = !mIsVideoHandling && !mIsPicHandling;

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//			, "mod_file_recorder: VideoFlvRecorder::SetVideoHandling() recorder:%p, isHandling:%d, result:%d, mIsVideoHandling:%d, mIsPicHandling:%d, mpCallback:%p\n"
//			, this, isHandling, result, mIsVideoHandling, mIsPicHandling, mpCallback);

	if( result && mpCallback ) {
		mpCallback->OnStop(this);
	}
	switch_mutex_unlock(mpMutex);

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//			, "mod_file_recorder: VideoFlvRecorder::SetVideoHandling() stop recorder:%p, isHandling:%d\n"
//			, this, isHandling);
}

// 设置监控截图是否正在处理
void VideoFlvRecorder::SetPicHandling(bool isHandling)
{
//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//			, "mod_file_recorder: VideoFlvRecorder::SetPicHandling() start recorder:%p, isHandling:%d\n"
//			, this, isHandling);

	switch_mutex_lock(mpMutex);
	mIsPicHandling = isHandling;
	bool result = !mIsVideoHandling && !mIsPicHandling;

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//			, "mod_file_recorder: VideoFlvRecorder::SetPicHandling() recorder:%p, isHandling:%d, result:%d, mIsVideoHandling:%d, mIsPicHandling:%d, mpCallback:%p\n"
//			, this, isHandling, result, mIsVideoHandling, mIsPicHandling, mpCallback);

	if( result && mpCallback ) {
		mpCallback->OnStop(this);
	}
	switch_mutex_unlock(mpMutex);

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//			, "mod_file_recorder: VideoFlvRecorder::SetPicHandling() stop recorder:%p, isHandling:%d\n"
//			, this, isHandling);
}

// 监制截图线程处理
bool VideoFlvRecorder::RecordPicture2FileProc()
{
	bool result = false;
//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//			, "mod_file_recorder: VideoFlvRecorder::RecordPicture2FileProc() start, handle:%p\n"
//			, mpHandle);

	if (mIsRecord && mIsPicHandling)
	{
		if (mlPicBuildTime < getCurrentTime())
		{
			result = true;

			// log for test
//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//						, "mod_file_recorder: VideoFlvRecorder::RecordPicture2FileProc() BuildPicture start\n");

			// 生成图片
			bool bBuildPicResult = false;
			video_frame_t *frame = PopPicBuffer();
			if (NULL != frame && NULL != frame->buffer)
			{
				if (switch_buffer_inuse(frame->buffer) > 0) {
					// 生成图片
					bBuildPicResult = BuildPicture(frame, mpPicDataBuffer, miPicDataBufferSize);
				}

				// 回收buffer
				RecycleFreeBuffer(frame);
			}

			// 更新下一周期处理时间
			mlPicBuildTime = getCurrentTime() + miPicInterval;

			// log for test
//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//						, "mod_file_recorder: VideoFlvRecorder::RecordPicture2FileProc() BuildPicture finish, buffer:%p, result:%d\n"
//						, mpPicBuffer, bBuildPicResult);
		}
	}

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//			, "mod_file_recorder: VideoFlvRecorder::RecordPicture2FileProc() stop, handle:%p\n"
//			, mpHandle);

	return result;
}

// 判断是否i帧
bool VideoFlvRecorder::IsIFrame(const uint8_t* data, switch_size_t inuse)
{
	bool isIFrame = false;
	static const unsigned char nalHead[] = {0x00, 0x00, 0x00, 0x01};
	static const int nalHeadSize = sizeof(nalHead);

	static const int nalSpsSize = 45;
	static const int nalPpsSize = 4;
	static const int nalTypeSize = 1;

	static const unsigned char nalIdcBit = 0x60;
	static const unsigned char nalIdcBitOff = 5;
	static const unsigned char nalTypeBit = 0x1F;

	int currNalOffset = 0;
	unsigned char nalTypeCount = 0;
	unsigned char nalType = 0;

	if (inuse > nalHeadSize)
	{
		if (0 == memcmp(nalHead, data, nalHeadSize))
		{
			nalTypeCount = data[currNalOffset + nalHeadSize] & nalIdcBit;
			nalTypeCount = nalTypeCount >> 5;
		}
		else {
//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//				, "mod_file_recorder: VideoFlvRecorder::IsIFrame() memcmp() false\n");
		}

		unsigned char i = 0;
		for (i = 0; i < nalTypeCount; i++)
		{
			if (inuse < currNalOffset + nalHeadSize + nalTypeSize) {
				// end
				break;
			}

			if (0 == memcmp(nalHead, data + currNalOffset, nalHeadSize))
			{
				nalType = data[currNalOffset + nalHeadSize] & nalTypeBit;
				if (5 == nalType) {
					// IDR (i frame)
					isIFrame = true;

//					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//						, "mod_file_recorder: VideoFlvRecorder::IsIFrame() nalType:%d, isIFrame:%d\n"
//						, nalType, isIFrame);
					break;
				}
				else if (7 == nalType) {
					// SPS set it i frame
					isIFrame = true;
					break;

					// SPS (offset SPS size + nal head size)
//					currNalOffset += nalHeadSize + nalSpsSize;
//
//					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//						, "mod_file_recorder: VideoFlvRecorder::IsIFrame() nalType:%d, nalHeadSize:%d, nalSpsSize:%d, currNalOffset:%d\n"
//						, nalType, nalHeadSize, nalSpsSize, currNalOffset);
				}
				else if (8 == nalType) {
					// PPS (offset PPS size + nal head size)
					currNalOffset += nalHeadSize + nalPpsSize;

//					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//						, "mod_file_recorder: VideoFlvRecorder::IsIFrame() nalType:%d, nalHeadSize:%d, nalPpsSize:%d, currNalOffset:%d\n"
//						, nalType, nalHeadSize, nalPpsSize, currNalOffset);
				}
				else {
//					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//						, "mod_file_recorder: VideoFlvRecorder::IsIFrame() nalType:%d, currNalOffset:%d\n"
//						, nalType, currNalOffset);
				}
			}
			else {
//				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoFlvRecorder::IsIFrame() memcmp() false, currNalOffset:%d, nalHeadSize:%d\n"
//					, currNalOffset, nalHeadSize);
			}
		}
	}
	else {
//		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//			, "mod_file_recorder: VideoFlvRecorder::IsIFrame() inuse > nalHeadSize false, inuse:%d, nalHeadSize:%d\n"
//			, inuse, nalHeadSize);
	}

	return isIFrame;
}

switch_status_t VideoFlvRecorder::buffer_h264_nalu(switch_frame_t *frame, switch_buffer_t* nalu_buffer, switch_bool_t& nalu_28_start, uint8_t &nalu_type)
{
//	uint8_t nalu_type = 0;
	uint8_t *data = (uint8_t *)frame->data;
	uint8_t nalu_hdr = *data;
	uint8_t sync_bytes[] = {0, 0, 0, 1};
	uint8_t slice_sync_bytes[] = {0, 0, 1};
	switch_buffer_t *buffer = nalu_buffer;

	nalu_type = nalu_hdr & 0x1f;

	/* hack for phones sending sps/pps with frame->m = 1 such as grandstream */
	switch_mutex_lock(mpPicMutex);
	if (nalu_type == 7) {
		switch_buffer_zwrite(mpSpsBuffer, (const void *)frame->data, frame->datalen);
	} else if (nalu_type == 8) {
		switch_buffer_zwrite(mpPpsBuffer, (const void *)frame->data, frame->datalen);
	}
	switch_mutex_unlock(mpPicMutex);

	if ((nalu_type == 7 || nalu_type == 8) && frame->m) {
		frame->m = SWITCH_FALSE;
		nalu_28_start = SWITCH_FALSE;

		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG,
				"mod_file_recorder: VideoFlvRecorder::buffer_h264_nalu(), "
				"frame: %p, nalu_type: %d, first_nalu: %d, m: %d, datalen: %d, bufferSize: %d \n",
				frame, nalu_type, frame->first_nalu, frame->m, frame->datalen, switch_buffer_inuse(buffer));
		return SWITCH_STATUS_SUCCESS;
	}

	if (nalu_type == 28) { // 0x1c FU-A
		int start = *(data + 1) & 0x80;
		int end = *(data + 1) & 0x40;

		nalu_type = *(data + 1) & 0x1f;

		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG,
				"mod_file_recorder: VideoFlvRecorder::buffer_h264_nalu(), "
				"frame: %p, nalu_type: %d, first_nalu: %d, m: %d, datalen: %d, bufferSize: %d \n",
				frame, nalu_type, frame->first_nalu, frame->m, frame->datalen, switch_buffer_inuse(buffer));

		if (start && end) return SWITCH_STATUS_RESTART;

		if (start) {
			if ( nalu_28_start == SWITCH_TRUE ) {
				nalu_28_start = SWITCH_FALSE;
				switch_buffer_zero(buffer);
			}
		} else if (end) {
			// Add by Max
			if ( !start && !nalu_28_start ) {
				//  skip error slice frame
				return SWITCH_STATUS_SUCCESS;
			}
			nalu_28_start = SWITCH_FALSE;
		} else if ( nalu_28_start == SWITCH_FALSE ) {
			return SWITCH_STATUS_RESTART;
		}

		if (start) {
			uint8_t nalu_idc = (nalu_hdr & 0x60) >> 5;
			nalu_type |= (nalu_idc << 5);

			if ( (nalu_type & 0x1f) == 5 ) {
				uint8_t *tmp = NULL;
				switch_size_t size = 0;
				size = switch_buffer_peek_zerocopy(mpSpsBuffer, (const void **)&tmp);
				if ( size > 0 ) {
					switch_buffer_write(buffer, sync_bytes, sizeof(sync_bytes));
					switch_buffer_write(buffer, tmp, size);
				}
				size = switch_buffer_peek_zerocopy(mpPpsBuffer, (const void **)&tmp);
				if ( size > 0 ) {
					switch_buffer_write(buffer, sync_bytes, sizeof(sync_bytes));
					switch_buffer_write(buffer, tmp, size);
				}
			}

			if ( frame->first_nalu ) {
				// If it is first slice of frame, write Nalu Start Code(00, 00, 00, 01)
				switch_buffer_write(buffer, sync_bytes, sizeof(sync_bytes));
			} else {
				// If it is not the first slice of frame, write Slice Start Code(00, 00, 01)
				switch_buffer_write(buffer, slice_sync_bytes, sizeof(slice_sync_bytes));
			}

			switch_buffer_write(buffer, &nalu_type, 1);

			nalu_28_start = SWITCH_TRUE;
		}

		switch_buffer_write(buffer, (void *)(data + 2), frame->datalen - 2);
	} else if (nalu_type == 24) { // 0x18 STAP-A
		uint16_t nalu_size;
		int left = frame->datalen - 1;

		data++;

	again:
		if (left > 2) {
			nalu_size = ntohs(*(uint16_t *)data);
			data += 2;
			left -= 2;

			if (nalu_size > left) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
						, "mod_file_recorder: VideoFlvRecorder::buffer_h264_nalu() INVALID PACKET, recorder:%p\n"
						, this);
				switch_buffer_zero(buffer);
				return SWITCH_STATUS_FALSE;
			}

			nalu_hdr = *data;
			nalu_type = nalu_hdr & 0x1f;

			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG,
					"mod_file_recorder: VideoFlvRecorder::buffer_h264_nalu(), "
					"frame: %p, nalu_type: %d, first_nalu: %d, m: %d, datalen: %d, bufferSize: %d \n",
					frame, nalu_type, frame->first_nalu, frame->m, frame->datalen, switch_buffer_inuse(buffer));

			if ( nalu_type == 5 ) {
				uint8_t *tmp = NULL;
				switch_size_t size = 0;
				size = switch_buffer_peek_zerocopy(mpSpsBuffer, (const void **)&tmp);
				if ( size > 0 ) {
					switch_buffer_write(buffer, sync_bytes, sizeof(sync_bytes));
					switch_buffer_write(buffer, tmp, size);
				}
				size = switch_buffer_peek_zerocopy(mpPpsBuffer, (const void **)&tmp);
				if ( size > 0 ) {
					switch_buffer_write(buffer, sync_bytes, sizeof(sync_bytes));
					switch_buffer_write(buffer, tmp, size);
				}
			}

			switch_buffer_write(buffer, sync_bytes, sizeof(sync_bytes));
			switch_buffer_write(buffer, (void *)data, nalu_size);
			data += nalu_size;
			left -= nalu_size;
			goto again;
		}
	} else {
//		switch_buffer_write(buffer, sync_bytes, sizeof(sync_bytes));
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG,
				"mod_file_recorder: VideoFlvRecorder::buffer_h264_nalu(), "
				"frame: %p, nalu_type: %d, first_nalu: %d, m: %d, datalen: %d, bufferSize: %d \n",
				frame, nalu_type, frame->first_nalu, frame->m, frame->datalen, switch_buffer_inuse(buffer));

		if ( (nalu_type & 0x1f) == 5 ) {
			uint8_t *tmp = NULL;
			switch_size_t size = 0;
			size = switch_buffer_peek_zerocopy(mpSpsBuffer, (const void **)&tmp);
			if ( size > 0 ) {
				switch_buffer_write(buffer, sync_bytes, sizeof(sync_bytes));
				switch_buffer_write(buffer, tmp, size);
			}
			size = switch_buffer_peek_zerocopy(mpPpsBuffer, (const void **)&tmp);
			if ( size > 0 ) {
				switch_buffer_write(buffer, sync_bytes, sizeof(sync_bytes));
				switch_buffer_write(buffer, tmp, size);
			}
		}

		if ( frame->first_nalu ) {
			// If it is first slice of frame, write Nalu Start Code(00, 00, 00, 01)
			switch_buffer_write(buffer, sync_bytes, sizeof(sync_bytes));
		} else {
			// If it is not the first slice of frame, write Slice Start Code(00, 00, 01)
			switch_buffer_write(buffer, slice_sync_bytes, sizeof(slice_sync_bytes));
		}
		switch_buffer_write(buffer, frame->data, frame->datalen);
		nalu_28_start = SWITCH_FALSE;
	}

	if (frame->m) {
		nalu_28_start = SWITCH_FALSE;
	}

	return SWITCH_STATUS_SUCCESS;
}

// 重新建Nalu缓冲
bool VideoFlvRecorder::RenewNaluBuffer()
{
	bool result = false;
	if (IsRecord()
		&& NULL != mpMemoryPool)
	{
		mpNaluBuffer = GetFreeBuffer();
		result = (NULL != mpNaluBuffer);
	}
	return result;
}

// 执行关闭shell
void VideoFlvRecorder::RunCloseShell()
{
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
					, "mod_file_recorder: VideoFlvRecorder::RunCloseShell() start, recorder:%p\n"
					, this);

	if (strlen(mcFinishShell) > 0)
	{
		// get mp4Path
		char mp4Path[MAX_PATH_LENGTH] = {0};
		GetMp4FilePath(mp4Path, mcVideoDstDir);

		// get userId
		char userId[MAX_PATH_LENGTH] = {0};
		GetUserIdWithFilePath(userId, mcVideoRecPath);

		// get siteId
		char siteId[MAX_PATH_LENGTH] = {0};
		GetSiteIdWithFilePath(siteId, mcVideoRecPath);

		// get startTime
		char startTime[MAX_PATH_LENGTH] = {0};
		GetStartTimeWithFilePath(startTime, mcVideoRecPath);
		char startStandardTime[32] = {0};
		GetStandardTime(startStandardTime, startTime);

		// print param
//		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//						, "mod_file_recorder: VideoFlvRecorder::RunCloseShell() run close shell, 0:%s, 1:%s, 2:%s, 3:%s, 4:%s, 5:%s, 6:%s\n"
//						, mcFinishShell, mcVideoRecPath, mp4Path, mcPicDstPath, userId, siteId, startStandardTime);

		// build shell处理文件
		// shell recPath mp4Path jpgPath userId siteId startTime
		char cmd[MAX_PATH_LENGTH] = {0};
		snprintf(cmd, sizeof(cmd), "%s \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\""
				, mcFinishShell, mcVideoRecPath, mp4Path, mcPicDstPath, mcPicRecPath
				, userId, siteId, startStandardTime);

		// log
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO
					, "mod_file_recorder: VideoFlvRecorder::RunCloseShell() transcode to mp4 start, recorder:%p, cmd:%s\n"
					, this, cmd);

		// run shell
//		int code = system(cmd);
		bool result = SendCommand(cmd);

		// log
		if (result) {
			// success
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO
						, "mod_file_recorder: VideoFlvRecorder::RunCloseShell() transcode to mp4 success, recorder:%p, cmd:%s\n"
						, this, cmd);
		}
		else {
			// error
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
						, "mod_file_recorder: VideoFlvRecorder::RunCloseShell() transcode to mp4 fail! recorder:%p, cmd:%s\n"
						, this, cmd);
		}
	}

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
					, "mod_file_recorder: RunCloseShell() end, recorder:%p\n"
					, this);
}

// 根据源文件路径获取SiteID
bool VideoFlvRecorder::GetSiteIdWithFilePath(char* buffer, const char* srcPath)
{
	bool result = false;
	const int siteIdIndex = 1;

	char fileName[MAX_PATH_LENGTH] = {0};
	if (GetFileNameWithoutExt(fileName, srcPath)
		&& GetParamWithFileName(buffer, fileName, siteIdIndex))
	{
		result = true;
	}
	return result;
}

// 根据源文件路径获取起始时间
bool VideoFlvRecorder::GetStartTimeWithFilePath(char* buffer, const char* srcPath)
{
	bool result = false;
	const int startTimeIndex = 2;

	char fileName[MAX_PATH_LENGTH] = {0};
	if (GetFileNameWithoutExt(fileName, srcPath)
		&& GetParamWithFileName(buffer, fileName, startTimeIndex))
	{
		result = true;
	}
	return result;
}

// 获取标准时间格式
bool VideoFlvRecorder::GetStandardTime(char* buffer, const char* srcTime)
{
	bool result = false;
	if (NULL != srcTime && strlen(srcTime) == 14)
	{
		int i = 0;
		int j = 0;
		for (; srcTime[i] != 0; i++, j++)
		{
			if (i == 4
				|| i == 6)
			{
				// 日期分隔
				buffer[j++] = '-';
			}
			else if (i == 8)
			{
				// 日期与时间之间的分隔
				buffer[j++] = ' ';
			}
			else if (i == 10
					|| i == 12)
			{
				// 时间分隔
				buffer[j++] = ':';
			}
			buffer[j] = srcTime[i];
		}
		buffer[j] = 0;

		result = true;
	}
	return result;
}

// 获取用户ID
bool VideoFlvRecorder::GetUserIdWithFilePath(char* buffer, const char* srcPath)
{
	bool result = false;
	const int userIdIndex = 0;

	char fileName[MAX_PATH_LENGTH] = {0};
	if (GetFileNameWithoutExt(fileName, srcPath)
		&& GetParamWithFileName(buffer, fileName, userIdIndex))
	{
		result = true;
	}
	return result;
}

// 获取没有后缀的文件名
bool VideoFlvRecorder::GetFileNameWithoutExt(char* buffer, const char* filePath)
{
	bool result = false;

	if (NULL != filePath && filePath[0] != '\0')
	{
		// get file name
		const char* name = strrchr(filePath, '/');
		if (NULL == name) {
			name = strrchr(filePath, '\\');
		}
		// offset '/' or '\\'
		if (NULL != name) {
			name++;
		}
		else {
			name = filePath;
		}

		// get extension
		if (NULL != name) {
			const char* ext = strrchr(name, '.');
			if (NULL != ext) {
				strncpy(buffer, name, ext - name);
			}
			else {
				strcpy(buffer, name);
			}
			result = true;
		}
	}
	return result;
}

// 获取文件名中的第index个参数(从0开始)
bool VideoFlvRecorder::GetParamWithFileName(char* buffer, const char* fileName, int index)
{
	bool result = false;

	// define separator
	static const char separator = '_';

	if (NULL != fileName && fileName[0] != '\0')
	{
		// get param
		int i = 0;
		const char* begin = fileName;
		while (NULL != begin) {
			const char* end = strchr(begin, separator);
			if (i == index) {
				if (NULL != end) {
					// found
					strncpy(buffer, begin, end - begin);
				}
				else {
					// end of param
					strcpy(buffer, begin);
				}
				result = true;
				break;
			}
			else {
				if (NULL != end) {
					// go to next param
					begin = end + 1;
				}
				else {
					// not found
					break;
				}
			}
			i++;
		}
	}

	return result;
}

// 生成录制文件路径
bool VideoFlvRecorder::BuildFlvFilePath(const char* srcPath)
{
	bool result = false;
	if (NULL != srcPath && srcPath[0] != '\0')
	{
		strcat(mcVideoRecPath, srcPath);
		result = true;
	}
	return result;
}

// 获取mp4文件路径
bool VideoFlvRecorder::GetMp4FilePath(char* mp4Path, const char* dir)
{
	bool result = false;
	if (NULL != dir && dir[0] != '\0')
	{
		int dirLen = strlen(dir);
		if (dirLen > 0)
		{
			// copy dir path
			strcpy(mp4Path, dir);
			if (dir[dirLen-1] != '/'
				&& dir[dirLen-1] != '\\')
			{
				strcat(mp4Path, "/");

			}

			result = true;
		}
	}
	return result;
}

// 把视频数据写入文件
bool VideoFlvRecorder::WriteVideoData2FlvFile(video_frame_t *frame)
{
	bool result = false;

	if (NULL != frame
		&& NULL != frame->buffer
		&& switch_buffer_inuse(frame->buffer) < miVideoDataBufferSize)
	{
		// 从buffer中读取数据
		miVideoDataBufferLen = switch_buffer_peek(frame->buffer, mpVideoDataBuffer, miVideoDataBufferSize);
		if (miVideoDataBufferLen > 0) {
//			// 由于这里读出数据，因此在这里判断是否为I帧
//			frame->isIFrame = IsIFrame(mpVideoDataBuffer, miVideoDataBufferLen);

			// 把数据写入flv文件
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG,
					"mod_file_recorder: VideoFlvRecorder::WriteVideoData2FlvFile(), "
					"timestamp: %d, miVideoDataBufferLen: %d \n",
					frame->timestamp, miVideoDataBufferLen);
			int writeResult = srs_flv_write_h264_raw_frames(mpFlvFile, frame->timestamp, (char*)mpVideoDataBuffer, miVideoDataBufferLen);
			if (0 == writeResult) {
				result = true;
			} else {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
						"mod_file_recorder: VideoFlvRecorder::WriteVideoData2FlvFile() fail, "
						"writeResult: %d, timestamp: %d, miVideoDataBufferLen: %d, "
						"data: 0x%02x,0x%02x,0x%02x,0x%02x,0x%02x \n",
						writeResult, frame->timestamp, miVideoDataBufferLen,
						(unsigned char*)mpVideoDataBuffer[0],
						(unsigned char*)mpVideoDataBuffer[1],
						(unsigned char*)mpVideoDataBuffer[2],
						(unsigned char*)mpVideoDataBuffer[3],
						(unsigned char*)mpVideoDataBuffer[4]);
			}
		}

		// 重置数据
		miVideoDataBufferLen = 0;
	}
	return result;
}

// 释放视频录制队列里所有buffer及空闲队列
void VideoFlvRecorder::DestroyVideoQueue()
{
	// 释放队列所有buffer
	if (NULL != mpVideoQueue) {
		video_frame_t* frame = NULL;
		while (SWITCH_STATUS_SUCCESS == switch_queue_trypop(mpVideoQueue, (void**)&frame))
		{
			RecycleFreeBuffer(frame);
		}
	}
}

// 生成截图的视频文件生成路径
bool VideoFlvRecorder::BuildPicRecFilePath(const char* srcPath, const char* dir)
{
	bool result = false;
	if (NULL != srcPath && srcPath[0] != '\0'
		&& NULL != dir && dir[0] != '\0')
	{
		int dirLen = strlen(dir);
		if (dirLen > 0)
		{
			// copy dir path
			strcpy(mcPicRecPath, dir);
			if (dir[dirLen-1] != '/'
				&& dir[dirLen-1] != '\\')
			{
				strcat(mcPicRecPath, "/");
			}

			// copy file user id
			char userId[MAX_PATH_LENGTH] = {0};
			if (GetUserIdWithFilePath(userId, srcPath))
			{
				strcat(mcPicRecPath, userId);
				strcat(mcPicRecPath, ".h264");

				result = true;
			}
		}
	}
	return result;
}

// 生成截图图片路径
bool VideoFlvRecorder::BuildPicFilePath(const char* srcPath, const char* dir)
{
	bool result = false;
	if (NULL != srcPath && srcPath[0] != '\0'
		&& NULL != dir && dir[0] != '\0')
	{
		int dirLen = strlen(dir);
		if (dirLen > 0)
		{
			// copy dir path
			strcpy(mcPicDstPath, dir);
			if (dir[dirLen-1] != '/'
				&& dir[dirLen-1] != '\\')
			{
				strcat(mcPicDstPath, "/");
			}

			// copy file user id
			char userId[MAX_PATH_LENGTH] = {0};
			if (GetUserIdWithFilePath(userId, srcPath))
			{
				strcat(mcPicDstPath, userId);
				strcat(mcPicDstPath, ".jpg");

				result = true;
			}
		}
	}
	return result;
}

// 重建pic缓冲
bool VideoFlvRecorder::RenewPicBuffer(video_frame_t* buffer)
{
	switch_mutex_lock(mpPicMutex);

	// 回收旧buffer
	if (NULL != mpPicBuffer) {
		RecycleFreeBuffer(mpPicBuffer);
		mpPicBuffer = NULL;
	}

	// 重设
	mpPicBuffer = buffer;

	switch_mutex_unlock(mpPicMutex);
	return true;
}

// pop出pic缓冲
VideoFlvRecorder::video_frame_t* VideoFlvRecorder::PopPicBuffer()
{
	video_frame_t* buffer = NULL;

	switch_mutex_lock(mpPicMutex);
	buffer = mpPicBuffer;
	mpPicBuffer = NULL;
	switch_mutex_unlock(mpPicMutex);
	return buffer;
}

// 生成截图
bool VideoFlvRecorder::BuildPicture(video_frame_t* frame, uint8_t* dataBuffer, switch_size_t dataBufLen)
{
	bool result = false;

	result = BuildPicRecFile(frame, dataBuffer, dataBufLen);
	result = result && RunPictureShell();

	return result;
}

// 生成截图的H264文件
bool VideoFlvRecorder::BuildPicRecFile(video_frame_t* frame, uint8_t* dataBuffer, switch_size_t dataBufLen)
{
	bool result = false;
	if (NULL != frame && NULL != frame->buffer
		&& switch_buffer_inuse(frame->buffer) > 0
		&& switch_buffer_inuse(frame->buffer) < dataBufLen)
	{
		// 打开文件
		FILE* pFile = fopen(mcPicRecPath, "w+b");
		if (NULL != pFile)
		{
			// 数据写入文件
			switch_size_t inuseSize = switch_buffer_inuse(frame->buffer);
			switch_size_t procSize = switch_buffer_peek(frame->buffer, dataBuffer, dataBufLen);
			if (procSize == inuseSize) {
				procSize = fwrite(dataBuffer, 1, procSize, pFile);
			}
			result = (procSize == inuseSize);

			// 关闭文件
			fclose(pFile);
			pFile = NULL;
		}
		else {
			// error
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
						, "mod_file_recorder: VideoFlvRecorder::BuildPicH264File() fail! recorder:%p, path:%s\n"
						, this, mcPicRecPath);
		}
	}

	// log
//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//		, "mod_file_recorder: VideoFlvRecorder::BuildPicH264File() result:%d, path:%s\n"
//		, result, mcPicRecPath);

	return result;
}

// 执行生成截图shell
bool VideoFlvRecorder::RunPictureShell()
{
	bool result = false;

	// build shell处理文件
	char cmd[MAX_PATH_LENGTH] = {0};
//	snprintf(cmd, sizeof(cmd), "%s %s %s"
//			, mcPicShell, mcPicRecPath, mcPicDstPath);
	snprintf(cmd, sizeof(cmd), "/usr/local/bin/ffmpeg -v error -i %s -y -s 240x180 -vframes 1 %s"
			, mcPicRecPath, mcPicDstPath);

	// log
//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//				, "mod_file_recorder: VideoFlvRecorder::RunPictureShell() transcode to jpeg start, cmd:%s\n"
//				, cmd);

	// run shell
//	int code = system(cmd);
//	result = (code >= 0);
//	result = true;
	result = SendCommand(cmd);

	// log
	if (result) {
		// log for test
//		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoFlvRecorder::RunPictureShell() transcode to jpeg success, cmd:%s\n"
//					, cmd);
	}
	else {
		// error
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
					, "mod_file_recorder: VideoFlvRecorder::RunPictureShell() transcode to jpeg fail! recorder:%p, cmd:%s\n"
					, this, cmd);
	}

	return result;
}

bool VideoFlvRecorder::SendCommand(const char* cmd) {
//	switch_log_printf(
//			SWITCH_CHANNEL_LOG,
//			SWITCH_LOG_DEBUG,
//			"mod_file_recorder: VideoFlvRecorder::SendCommand() cmd: \"%s\"\n",
//			cmd
//			);
	return file_record_send_command_lua(cmd);
}

bool VideoFlvRecorder::file_record_send_command(const char* cmd) {
	switch_status_t status = SWITCH_STATUS_FALSE;
	switch_event_t *event;
	if ( (status = switch_event_create_subclass(&event, SWITCH_EVENT_CUSTOM, FILE_RECORDER_EVENT_MAINT)) == SWITCH_STATUS_SUCCESS) {
		switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "cmd", cmd);
		status = switch_event_fire(&event);
	}

	if( status != SWITCH_STATUS_SUCCESS ) {
		switch_log_printf(
				SWITCH_CHANNEL_LOG,
				SWITCH_LOG_ERROR,
				"VideoFlvRecorder::file_record_send_command( "
				"[Send event Fail], "
				"this: %p, "
				"cmd: '%s' "
				") \n",
				this,
				cmd
				);
	}

	return status == SWITCH_STATUS_SUCCESS;
}

bool VideoFlvRecorder::file_record_send_command_lua(const char* cmd) {
	bool bFlag = true;

	char result[SWITCH_CMD_CHUNK_LEN];
	switch_stream_handle_t stream = { 0 };
//	SWITCH_STANDARD_STREAM(stream);

	memset(&stream, 0, sizeof(stream));
	stream.data = result;
	memset(stream.data, 0, SWITCH_CMD_CHUNK_LEN);
	stream.end = stream.data;
	stream.data_size = SWITCH_CMD_CHUNK_LEN;
	stream.write_function = switch_console_stream_write;
	stream.raw_write_function = switch_console_stream_raw_write;
	stream.alloc_len = SWITCH_CMD_CHUNK_LEN;
	stream.alloc_chunk = SWITCH_CMD_CHUNK_LEN;

	char path[2048] = {0};
	char url[2048] = {0};
	if( switch_url_encode(cmd, url, sizeof(url)) ) {
		snprintf(path, sizeof(path) - 1, "event_file_recorder.lua %s", url);
	}

//	switch_log_printf(
//			SWITCH_CHANNEL_LOG,
//			SWITCH_LOG_INFO,
//			"mod_file_recorder::VideoFlvRecorder::file_record_send_command_lua( "
//			"[LUA call], "
//			"this: %p, "
//			"path: %s "
//			") \n",
//			this,
//			path
//			);

	if( strlen(path) > 0 && switch_api_execute("lua", path, NULL, &stream) == SWITCH_STATUS_SUCCESS ) {
		bFlag = true;
//		switch_log_printf(
//				SWITCH_CHANNEL_LOG,
//				SWITCH_LOG_INFO,
//				"mod_file_recorder::VideoFlvRecorder::file_record_send_command_lua( "
//				"[LUA call OK], "
//				"this: %p, "
//				"stream.data: '%s' "
//				") \n",
//				this,
//				stream.data
//				);
	} else {
		switch_log_printf(
				SWITCH_CHANNEL_LOG,
				SWITCH_LOG_ERROR,
				"mod_file_recorder::VideoFlvRecorder::file_record_send_command_lua( "
				"[LUA call Fail], "
				"this: %p, "
				"stream.data: '%s' "
				") \n",
				this,
				stream.data
				);
	}

//	switch_safe_free(stream.data);

	return bFlag;
}

// 获取空闲buffer
VideoFlvRecorder::video_frame_t* VideoFlvRecorder::GetFreeBuffer()
{
	video_frame_t* frame = NULL;
	if (NULL != mpFreeBufferQueue)
	{
		if (SWITCH_STATUS_SUCCESS != switch_queue_trypop(mpFreeBufferQueue, (void**)&frame))
		{
			// 获取失败，尝试创建
			switch_buffer_t *buffer = NULL;
			if (SWITCH_STATUS_SUCCESS == switch_buffer_create(mpMemoryPool, &buffer, H264_BUFFER_SIZE))
			{
				frame = new video_frame_t;
				if (NULL != frame) {
					// 创建成功
					frame->buffer = buffer;
					// 统计共生成多少个buffer
					miCreateBufferCount++;
//					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//							, "mod_file_recorder: VideoFlvRecorder::GetFreeBuffer() recorder:%p, miCreateBufferCount:%d, frame:%p\n"
//							, this, miCreateBufferCount, frame);
				}
				else {
					// 创建失败
					switch_buffer_destroy(&buffer);
				}
			}
		}

		// 获取失败打log
		if (NULL == frame) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
						, "mod_file_recorder: VideoFlvRecorder::GetFreeBuffer() get free buffer fail! recorder:%p, mpFreeBufferQueue:%p, mpMemoryPool:%p\n"
						, this, mpFreeBufferQueue, mpMemoryPool);
		}
	}

	return frame;
}

// 回收空闲buffer
void VideoFlvRecorder::RecycleFreeBuffer(VideoFlvRecorder::video_frame_t* frame)
{
//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//			, "mod_file_recorder: VideoFlvRecorder::RecycleFreeBuffer() start! frame:%p, mpFreeBufferQueue:%p\n"
//			, frame, mpFreeBufferQueue);

	if (NULL != frame
		&& NULL != mpFreeBufferQueue)
	{
		// log
//		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//				, "mod_file_recorder: VideoFlvRecorder::RecycleFreeBuffer() recycle free buffer success! frame:%p\n"
//				, frame);

		frame->reset();
		switch_queue_push(mpFreeBufferQueue, frame);
	}
	else {
		// log
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
				, "mod_file_recorder: VideoFlvRecorder::RecycleFreeBuffer() recycle free buffer error! frame:%p, mpFreeBufferQueue:%p\n"
				, frame, mpFreeBufferQueue);
	}
}

// 释放空闲队列里所有buffer及空闲队列
void VideoFlvRecorder::DestroyFreeBufferQueue()
{
	// 释放队列里所有buffer
	if (NULL != mpFreeBufferQueue) {
		video_frame_t* frame = NULL;
		while (SWITCH_STATUS_SUCCESS == switch_queue_trypop(mpFreeBufferQueue, (void**)&frame))
		{
			if (NULL != frame->buffer) {
				switch_buffer_t* buffer = frame->buffer;
				switch_buffer_destroy(&buffer);
				frame->buffer = buffer;
			}
			delete frame;
		}
	}
}

