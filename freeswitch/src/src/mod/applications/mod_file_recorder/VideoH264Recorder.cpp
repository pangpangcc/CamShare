/*
 * VideoH264Recorder.cpp
 *
 *  Created on: 2016-04-26
 *      Author: Samson.Fan
 * Description: 录制h264视频文件及监控截图
 */

#include "VideoH264Recorder.h"
#include "CommonFunc.h"

#define H264_BUFFER_SIZE (1024 * 128)

VideoH264Recorder::VideoH264Recorder()
{
//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "mod_file_recorder: VideoH264Recorder::VideoH264Recorder()\n");

	ResetParam();
	mpMemoryPool = NULL;
}

VideoH264Recorder::~VideoH264Recorder()
{
//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "mod_file_recorder: VideoH264Recorder::~VideoH264Recorder()\n");

//	StopRecord();
}

// 重置参数
void VideoH264Recorder::ResetParam()
{
	mIsRecord = false;
	mHasStartRecord = false;
	mIsVideoHandling = false;
//	mpVideoMutex = NULL;
	memset(mcVideoRecPath, 0, sizeof(mcVideoRecPath));
	memset(mcVideoDstDir, 0, sizeof(mcVideoDstDir));
	memset(mcFinishShell, 0, sizeof(mcFinishShell));

	mpFile = NULL;
	mpHandle = NULL;

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
bool VideoH264Recorder::StartRecord(switch_file_handle_t *handle
		, const char *videoRecPath, const char* videoDstDir, const char* finishShell
		, const char *picRecDir, const char* picDstDir, const char* picShell, int picInterval)
{
//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//			, "mod_file_recorder: VideoH264Recorder::StartRecord() handle:%p, memory_pool:%p"
//			  ", videoRecPath:%s, videoDstDir:%s, finishShell:%s"
//			  ", picRecDir:%s, picDstDir:%s, picShell:%s, picInterval:%d\n"
//			, handle
//			, (NULL!= handle ? handle->memory_pool : NULL)
//			, videoRecPath, videoDstDir, finishShell
//			, picRecDir, picDstDir, picShell, picInterval);

	// 确保不是正在运行，且目录没有问题
	if (!mIsRecord
		&& BuildH264FilePath(videoRecPath)
		&& IsDirExist(videoDstDir))
	{
		bool success = true;

		strcpy(mcFinishShell, finishShell);
		strcpy(mcVideoDstDir, videoDstDir);

//		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//				, "mod_file_recorder: VideoH264Recorder::StartRecord() mcVideoRecPath:%s, mcVideoDstDir:%s, mcFinishShell:%s\n"
//				, mcVideoRecPath, mcVideoDstDir, mcFinishShell);

		// 创建内存池
		if (success) {
			success = success
					&& (SWITCH_STATUS_SUCCESS == switch_core_new_memory_pool(&mpMemoryPool));

//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//							, "mod_file_recorder: VideoH264Recorder::StartRecord() mpMemoryPool:%p\n"
//							, mpMemoryPool);
		}

		// 创建状态锁
		if( success ) {
			success = (SWITCH_STATUS_SUCCESS
							== switch_mutex_init(&mpMutex, SWITCH_MUTEX_NESTED, mpMemoryPool));
		}


		// 记录句柄
		if (success) {
			mpHandle = handle;
		}

		// 打开文件
		if (success) {
			mpFile = fopen(mcVideoRecPath, "w+b");
			success = success && (NULL != mpFile);

			if (!success) {
				// error
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
					, "mod_file_recorder: VideoH264Recorder::StartRecord() open file fail, handle:%p, videoRecPath:%s\n"
					, handle, videoRecPath);
			}

//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//											, "mod_file_recorder: VideoH264Recorder::StartRecord() mpFile:%p, success:%d\n"
//											, mpFile, success);
		}

		// 打开文件成功
		if(success)
		{
			// 标记为已经打开文件
			mIsRecord = true;
			mHasStartRecord = mIsRecord;

//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//							, "mod_file_recorder: VideoH264Recorder::StartRecord() load video recorder source start\n");

			// 创建空闲buffer队列
			success = success
					&& (SWITCH_STATUS_SUCCESS
							== switch_queue_create(&mpFreeBufferQueue, SWITCH_CORE_QUEUE_LEN, mpMemoryPool));

//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//							, "mod_file_recorder: VideoH264Recorder::StartRecord() load free buffer queue, success:%d\n"
//							, success);

			// 创建Nalu缓冲
			success = success && RenewNaluBuffer();

//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//							, "mod_file_recorder: VideoH264Recorder::StartRecord() renew nalu buffer, success:%d\n"
//							, success);

			// 创建缓存队列
			success = success
					&& (SWITCH_STATUS_SUCCESS
							== switch_queue_create(&mpVideoQueue, SWITCH_CORE_QUEUE_LEN, mpMemoryPool));
//			success = success
//					&& (SWITCH_STATUS_SUCCESS
//							== switch_mutex_init(&mpVideoMutex, SWITCH_MUTEX_NESTED, mpMemoryPool));

//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//							, "mod_file_recorder: VideoH264Recorder::StartRecord() load video queue, success:%d\n"
//							, success);

			// 创建视频处理缓存
			miVideoDataBufferLen = 0;
			miVideoDataBufferSize = H264_BUFFER_SIZE;
			success = success
					&& (NULL != (mpVideoDataBuffer = (uint8_t*)switch_core_alloc(mpMemoryPool, miVideoDataBufferSize)));


//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//							, "mod_file_recorder: VideoH264Recorder::StartRecord() load video thread, success:%d\n"
//							, success);

			// 截图处理
			if (success)
			{
				if (picInterval > 0
					&& BuildPicH264FilePath(videoRecPath, picRecDir)
					&& BuildPicFilePath(videoRecPath, picDstDir)
					&& strlen(picShell) > 0)
				{
					// 设置参数
					miPicInterval = picInterval;
					strcpy(mcPicShell, picShell);

//					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//									, "mod_file_recorder: VideoH264Recorder::StartRecord() load cut picture start, success:%d\n"
//									, success);

					// 创建锁
					success = success
							&& (SWITCH_STATUS_SUCCESS
									== switch_mutex_init(&mpPicMutex, SWITCH_MUTEX_NESTED, mpMemoryPool));

					// 创建buffer
					miPicDataBufferSize = H264_BUFFER_SIZE;
					success = success
							&& (NULL != (mpPicDataBuffer = (uint8_t*)switch_core_alloc(mpMemoryPool, miPicDataBufferSize)));

//					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//									, "mod_file_recorder: VideoH264Recorder::StartRecord() load cut picture thread, success:%d\n"
//									, success);
				}
				else
				{
					// log
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
						, "mod_file_recorder: VideoH264Recorder::StartRecord() handle:%p, picInterval:%d, picRecDir:%s, picDstDir:%s"
						  ", picShell:%s, videoRecPath:%s\n"
						, handle, picInterval, picRecDir, picDstDir
						, picShell, videoRecPath);

					if (!IsDirExist(picRecDir)) {
						// 目录不存在
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
							, "mod_file_recorder: VideoH264Recorder::StartRecord() picRecDir:%s not exist\n"
							, picRecDir);
					}

					if (!IsDirExist(picDstDir)) {
						// 目录不存在
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
							, "mod_file_recorder: VideoH264Recorder::StartRecord() picDstDir:%s not exist\n"
							, picDstDir);
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
				, "mod_file_recorder: VideoH264Recorder::StartRecord() fail! videoRecPath:%s, videoDstDir:%s, handle:%p"
				  ", mpFile:%p, mpMemoryPool:%p, mpVideoQueue:%p\n"
				, videoRecPath, videoDstDir, handle
				, mpFile, mpMemoryPool, mpVideoQueue);

		if (!IsDirExist(videoDstDir)) {
			// 目录不存在
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
				, "mod_file_recorder: VideoH264Recorder::StartRecord() videoDstDir:%s not exist\n"
				, videoDstDir);
		}

		// 释放资源
		StopRecord();
	}

	// 打log
//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//			, "mod_file_recorder: VideoH264Recorder::StartRecord() finish, handle:%p, mIsRecord:%d, videoRecPath:%s\n"
//			, handle, mIsRecord, videoRecPath);

	return mIsRecord;
}

// 停止录制
void VideoH264Recorder::StopRecord()
{
	// log for test
//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//			, "mod_file_recorder: VideoH264Recorder::StopRecord() start, handle:%p, mpMemoryPool:%p, mpFile:%p, mIsRecord:%d\n"
//			, mpHandle, mpMemoryPool, mpFile, mIsRecord);

//	switch_mutex_lock(mpVideoMutex);
	mIsRecord = false;
//	switch_mutex_unlock(mpVideoMutex);
}

// 重置(包括重置参数及执行close_shell)
void VideoH264Recorder::Reset()
{
//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//				, "mod_file_recorder: VideoH264Recorder::Reset() start, isRecord:%d\n"
//				, mIsRecord);

	// 把剩余的VideoBuffer写入文件
	PutVideoBuffer2FileProc();

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoH264Recorder::Reset() over thread\n");

	// 关闭文件
	if(NULL != mpFile)
	{
		fclose(mpFile);
		mpFile = NULL;
	}

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoH264Recorder::Reset() close file\n");

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
//					, "mod_file_recorder: VideoH264Recorder::Reset() destroy mpNaluBuffer\n");


	// 释放空闲buffer队列
	DestroyFreeBufferQueue();

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//				, "mod_file_recorder: VideoH264Recorder::Reset() running close shell, hasRecord:%d, handle:%p, path:%s, mcFinishShell:%s\n"
//				, mHasStartRecord, mpHandle, mcVideoRecPath, mcFinishShell);

	// 之前状态为running，执行关闭shell
	if (mHasStartRecord)
	{
		// log for test
//		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//				, "mod_file_recorder: VideoH264Recorder::Reset() runing close shell, handle:%p\n"
//				, mpHandle);
		RunCloseShell();
	}

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//			, "mod_file_recorder: VideoH264Recorder::Reset() finish, handle:%p\n"
//			, mpHandle);

	// 重置参数
	switch_mutex_lock(mpMutex);
	ResetParam();
	switch_mutex_unlock(mpMutex);

	// 释放内存池
	if (NULL != mpMemoryPool)
	{
		switch_core_destroy_memory_pool(&mpMemoryPool);
		mpMemoryPool = NULL;
	}
}

void VideoH264Recorder::SetCallback(IVideoRecorderCallback* callback) {
	mpCallback = callback;
}

// 录制视频frame
bool VideoH264Recorder::RecordVideoFrame(switch_frame_t *frame)
{
	bool bFlag = true;

	// 加锁，防止线程已经停止，但本函数只处理一半，仍向队列插入数据
//	switch_mutex_lock(mpVideoMutex);

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoH264Recorder::RecordVideoFrame() start handle:%p\n"
//					, mpHandle);

	if (mIsRecord)
	{
//		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//				, "mod_file_recorder: VideoH264Recorder::RecordVideoFrame() handle:%p, datalen:%d\n"
//				, mpHandle,	frame->datalen);

		// 解析h264包
		switch_status_t status = buffer_h264_nalu(frame, mpNaluBuffer, mbNaluStart);

		if (status == SWITCH_STATUS_RESTART)
		{
			// 等待关键帧
			switch_set_flag(frame, SFF_WAIT_KEY_FRAME);
			switch_buffer_zero(mpNaluBuffer);
			mbNaluStart = SWITCH_FALSE;
			bFlag = true;

			// log for test
//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoH264Recorder::RecordVideoFrame() handle:%p, SWITCH_STATUS_RESTART, mcVideoRecPath:%s\n"
//					, mpHandle, mcVideoRecPath);

		}
		else if (frame->m && switch_buffer_inuse(mpNaluBuffer) > 0)
		{
			// 增加到缓存队列
			switch_queue_push(mpVideoQueue, mpNaluBuffer);

			// 创建新Nalu缓冲
			RenewNaluBuffer();

			// 重置标志位
			mbNaluStart = SWITCH_FALSE;
			bFlag = true;

			// log for test
//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoH264Recorder::RecordVideoFrame() push queue, handle:%p, mcVideoRecPath:%s\n"
//					, mpHandle, mcVideoRecPath);
		}
	}
	else {
		// error
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
						, "mod_file_recorder: VideoH264Recorder::RecordVideoFrame() stoped, mIsRecord:%d\n"
						, mIsRecord);

		bFlag = false;
	}

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoH264Recorder::RecordVideoFrame() end handle:%p\n"
//					, mpHandle);

//	switch_mutex_unlock(mpVideoMutex);

	return bFlag;
}

// 是否正在视频录制
bool VideoH264Recorder::IsRecord()
{
	return mIsRecord;
}

// 录制视频线程处理
bool VideoH264Recorder::RecordVideoFrame2FileProc()
{
	bool result = false;
	// log for test
//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//			, "mod_file_recorder: VideoH264Recorder::RecordVideoFrame2FileProc() start, handle:%p\n"
//			, mpHandle);

	if (mIsRecord)
	{
//		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//				, "mod_file_recorder: VideoH264Recorder::RecordVideoFrame2FileProc() pop start\n");

		// 获取缓存视频frame
		void* pop = NULL;
		while (SWITCH_STATUS_SUCCESS == switch_queue_trypop(mpVideoQueue, &pop))
		{
			result = true;
			bool bFreak = false;

			// 当前buffer在缓存的偏移位置
			size_t currBufferOffset = 0;

			switch_buffer_t* buffer = (switch_buffer_t*)pop;
			switch_size_t inuseSize = switch_buffer_inuse(buffer);
			if (inuseSize > 0)
			{
				if (miVideoDataBufferLen + inuseSize >= miVideoDataBufferSize) {
					// 缓存不足写入文件
					size_t writeLen = 0;
					if (WriteVideoData2File(mpVideoDataBuffer, miVideoDataBufferLen, writeLen)) {
						// 写入成功
						miVideoDataBufferLen = 0;
					}
					else {
						// 写入不成功
						if (writeLen > 0) {
							// buffer数据前移，并减少buffer的已用数据长度
							memcpy(mpVideoDataBuffer, mpVideoDataBuffer + writeLen, miVideoDataBufferLen - writeLen);
							miVideoDataBufferLen -= writeLen;
						}

						// error
//						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
//										, "mod_file_recorder: VideoH264Recorder::RecordVideoFrame2FileProc() write file fail, handle:%p, dataBufferLen:%d, writeLen:%d, mcVideoRecPath:%s\n"
//										, mpHandle, miVideoDataBufferLen, writeLen, mcVideoRecPath);
					}
				}

				// 偏移至本帧数据写入的位置
				currBufferOffset = miVideoDataBufferLen;

				// 视频数据写入缓存
				if (miVideoDataBufferLen + inuseSize < miVideoDataBufferSize) {
					switch_size_t procSize = switch_buffer_peek(buffer, mpVideoDataBuffer + miVideoDataBufferLen, miVideoDataBufferSize - miVideoDataBufferLen);
					if (procSize == inuseSize) {
						miVideoDataBufferLen += procSize;
					}
					else {
						// 写入缓存失败
						bFreak = true;
					}
				}
			}

			// log for test
//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoH264Recorder::RecordVideoFrame2FileProc() write finish, inuseSize:%d, bFreak:%d, mcVideoRecPath:%s\n"
//					, inuseSize, bFreak, mcVideoRecPath);

			bool isDestroyBuffer = true;
			if (!bFreak)
			{
				// handle i-frame
				bool isIFrame = IsIFrame(mpVideoDataBuffer + currBufferOffset, inuseSize);
				if (isIFrame && mIsPicHandling)
				{
					// 更新截图buffer & buffer不释放
					RenewPicBuffer(buffer);
					isDestroyBuffer = false;
				}

//				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//						, "mod_file_recorder: VideoH264Recorder::RecordVideoFrame2FileProc() check i-frame, i-frame:%d, isDestroyBuffer:%d\n"
//						, isIFrame, isDestroyBuffer);

//				if (!isDestroyBuffer)
//				{
//					// log for test
//					uint8_t* frameData = mpVideoDataBuffer + currBufferOffset;
//
//					const int logBufferSize = H264_BUFFER_SIZE;
//					char logBuffer[logBufferSize] = {0};
//					char logItemBuffer[32] = {0};
//					const int maxPrintSize = 64;
//					int theMaxPrintSize = (inuseSize > maxPrintSize ? maxPrintSize : inuseSize);
//					for (int i = 0; i < theMaxPrintSize; i++)
//					{
//						snprintf(logItemBuffer, sizeof(logItemBuffer)
//								, "0x%02x"
//								, frameData[i]);
//
//						strcat(logBuffer, logItemBuffer);
//
//						if (7 == i % 8
//							|| theMaxPrintSize == i + 1)
//						{
//							strcat(logBuffer, "\n");
//						}
//						else {
//							strcat(logBuffer, ", ");
//						}
//					}
//
//					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//						, "mod_file_recorder: VideoH264Recorder::RecordVideoFrame2FileProc()"
//						  " inuseSize:%d, bFreak:%d, isDestroyBuffer:%d, isIFrame:%d, mpPicBuffer:%p\n"
//						  "%s"
//						, inuseSize, bFreak, isDestroyBuffer, isIFrame, mpPicBuffer
//						, logBuffer);
//				}
			}

//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoH264Recorder::RecordVideoFrame2FileProc() do destroy buffer, isDestroyBuffer:%d\n"
//					, isDestroyBuffer);

			if (isDestroyBuffer)
			{
				// 回收buffer
				RecycleFreeBuffer(buffer);
			}

			// 读buffer数据失败
			if( bFreak ) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
						, "mod_file_recorder: VideoH264Recorder:RecordVideoFrame2FileProc() read buffer fail, handle:%p, inuse:%d, dataBufferLen:%d, dataBufferSize:%d, mcVideoRecPath:%s\n"
						, mpHandle, inuseSize, miVideoDataBufferLen, miVideoDataBufferSize, mcVideoRecPath);
				break;
			}

		}

//		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//				, "mod_file_recorder: VideoH264Recorder::RecordVideoFrame2FileProc() pop end\n");
	}

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//			, "mod_file_recorder: VideoH264Recorder::RecordVideoFrame2FileProc() end, handle:%p\n"
//			, mpHandle);

	return result;
}

void VideoH264Recorder::PutVideoBuffer2FileProc()
{
	// 把剩下的缓存写入文件
	if (miVideoDataBufferLen > 0)
	{
		size_t writeLen = 0;
		if (!WriteVideoData2File(mpVideoDataBuffer, miVideoDataBufferLen, writeLen)) {
			// 写入文件不成功
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
							, "mod_file_recorder: VideoH264Recorder::PutVideoBuffer2FileProc() write file fail, handle:%p, dataBufferLen:%d, writeLen:%d\n"
							, mpHandle, miVideoDataBufferLen, writeLen);
		}
	}
}

// 设置视频是否正在处理
void VideoH264Recorder::SetVideoHandling(bool isHandling)
{
	switch_mutex_lock(mpMutex);
	if( mIsVideoHandling != isHandling ) {
		mIsVideoHandling = isHandling;
	}
	bool result = !mIsVideoHandling && !mIsPicHandling;
	if( result && mpCallback ) {
		mpCallback->OnStop(this);
	}
	switch_mutex_unlock(mpMutex);
}

// 设置监控截图是否正在处理
void VideoH264Recorder::SetPicHandling(bool isHandling)
{
	switch_mutex_lock(mpMutex);
	if( mIsPicHandling != isHandling ) {
		mIsPicHandling = isHandling;
	}
	bool result = !mIsVideoHandling && !mIsPicHandling;
	if( result && mpCallback ) {
		mpCallback->OnStop(this);
	}
	switch_mutex_unlock(mpMutex);
}

// 监制截图线程处理
bool VideoH264Recorder::RecordPicture2FileProc()
{
	bool result = false;
//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//			, "mod_file_recorder: VideoH264Recorder::RecordPicture2FileProc() start, handle:%p\n"
//			, mpHandle);

	if (mIsRecord && mIsPicHandling)
	{
		if (mlPicBuildTime < getCurrentTime())
		{
			result = true;

			// log for test
//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//						, "mod_file_recorder: VideoH264Recorder::RecordPicture2FileProc() BuildPicture start\n");

			// 生成图片
			bool bBuildPicResult = false;
			switch_buffer_t* buffer = PopPicBuffer();
			if (NULL != buffer)
			{
				if (switch_buffer_inuse(buffer) > 0) {
					// 生成图片
					bBuildPicResult = BuildPicture(buffer, mpPicDataBuffer, miPicDataBufferSize);
				}

				// 回收buffer
				RecycleFreeBuffer(buffer);
			}

			// 更新下一周期处理时间
			mlPicBuildTime = getCurrentTime() + miPicInterval;

			// log for test
//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//						, "mod_file_recorder: VideoH264Recorder::RecordPicture2FileProc() BuildPicture finish, buffer:%p, result:%d\n"
//						, mpPicBuffer, bBuildPicResult);
		}
	}

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//			, "mod_file_recorder: VideoH264Recorder::RecordPicture2FileProc() stop, handle:%p\n"
//			, mpHandle);

	return result;
}

// 判断是否i帧
bool VideoH264Recorder::IsIFrame(const uint8_t* data, switch_size_t inuse)
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
//				, "mod_file_recorder: VideoH264Recorder::IsIFrame() memcmp() false\n");
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
//						, "mod_file_recorder: VideoH264Recorder::IsIFrame() nalType:%d, isIFrame:%d\n"
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
//						, "mod_file_recorder: VideoH264Recorder::IsIFrame() nalType:%d, nalHeadSize:%d, nalSpsSize:%d, currNalOffset:%d\n"
//						, nalType, nalHeadSize, nalSpsSize, currNalOffset);
				}
				else if (8 == nalType) {
					// PPS (offset PPS size + nal head size)
					currNalOffset += nalHeadSize + nalPpsSize;

//					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//						, "mod_file_recorder: VideoH264Recorder::IsIFrame() nalType:%d, nalHeadSize:%d, nalPpsSize:%d, currNalOffset:%d\n"
//						, nalType, nalHeadSize, nalPpsSize, currNalOffset);
				}
				else {
//					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//						, "mod_file_recorder: VideoH264Recorder::IsIFrame() nalType:%d, currNalOffset:%d\n"
//						, nalType, currNalOffset);
				}
			}
			else {
//				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoH264Recorder::IsIFrame() memcmp() false, currNalOffset:%d, nalHeadSize:%d\n"
//					, currNalOffset, nalHeadSize);
			}
		}
	}
	else {
//		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//			, "mod_file_recorder: VideoH264Recorder::IsIFrame() inuse > nalHeadSize false, inuse:%d, nalHeadSize:%d\n"
//			, inuse, nalHeadSize);
	}

	return isIFrame;
}

switch_status_t VideoH264Recorder::buffer_h264_nalu(switch_frame_t *frame, switch_buffer_t* nalu_buffer, switch_bool_t& nalu_28_start)
{
	uint8_t nalu_type = 0;
	uint8_t *data = (uint8_t *)frame->data;
	uint8_t nalu_hdr = *data;
	uint8_t sync_bytes[] = {0, 0, 0, 1};
	switch_buffer_t *buffer = nalu_buffer;

	nalu_type = nalu_hdr & 0x1f;

	/* hack for phones sending sps/pps with frame->m = 1 such as grandstream */
	if ((nalu_type == 7 || nalu_type == 8) && frame->m) frame->m = SWITCH_FALSE;

	if (nalu_type == 28) { // 0x1c FU-A
		int start = *(data + 1) & 0x80;
		int end = *(data + 1) & 0x40;

		nalu_type = *(data + 1) & 0x1f;

		if (start && end) return SWITCH_STATUS_RESTART;

		if (start) {
			if ( nalu_28_start == SWITCH_TRUE ) {
				nalu_28_start = SWITCH_FALSE;
				switch_buffer_zero(buffer);
			}
		} else if (end) {
			nalu_28_start = SWITCH_FALSE;
		} else if ( nalu_28_start == SWITCH_FALSE ) {
			return SWITCH_STATUS_RESTART;
		}

		if (start) {
			uint8_t nalu_idc = (nalu_hdr & 0x60) >> 5;
			nalu_type |= (nalu_idc << 5);

			switch_buffer_write(buffer, sync_bytes, sizeof(sync_bytes));
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
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "INVALID PACKET\n");
				switch_buffer_zero(buffer);
				return SWITCH_STATUS_FALSE;
			}

			nalu_hdr = *data;
			nalu_type = nalu_hdr & 0x1f;

			switch_buffer_write(buffer, sync_bytes, sizeof(sync_bytes));
			switch_buffer_write(buffer, (void *)data, nalu_size);
			data += nalu_size;
			left -= nalu_size;
			goto again;
		}
	} else {
		switch_buffer_write(buffer, sync_bytes, sizeof(sync_bytes));
		switch_buffer_write(buffer, frame->data, frame->datalen);
		nalu_28_start = SWITCH_FALSE;
	}

	if (frame->m) {
		nalu_28_start = SWITCH_FALSE;
	}

	return SWITCH_STATUS_SUCCESS;
}

// 重新建Nalu缓冲
bool VideoH264Recorder::RenewNaluBuffer()
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
void VideoH264Recorder::RunCloseShell()
{
//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoH264Recorder::RunCloseShell() start\n");

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
//						, "mod_file_recorder: VideoH264Recorder::RunCloseShell() run close shell, 0:%s, 1:%s, 2:%s, 3:%s, 4:%s, 5:%s, 6:%s\n"
//						, mcFinishShell, mcVideoRecPath, mp4Path, mcPicDstPath, userId, siteId, startStandardTime);

		// build shell处理文件
		// shell h264Path mp4Path jpgPath userId siteId startTime
		char cmd[MAX_PATH_LENGTH] = {0};
		snprintf(cmd, sizeof(cmd), "%s \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\""
				, mcFinishShell, mcVideoRecPath, mp4Path, mcPicDstPath, mcPicRecPath
				, userId, siteId, startStandardTime);

		// log for test
//		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoH264Recorder::RunCloseShell() transcode to mp4 start, cmd:%s\n"
//					, cmd);

		// run shell
//		int code = system(cmd);
		bool result = SendCommand(cmd);

		// log
		if (result) {
//		if( bFlag ) {
			// success
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
						, "mod_file_recorder: VideoH264Recorder::RunCloseShell() transcode to mp4 success, cmd:%s\n"
						, cmd);
		}
		else {
			// error
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
						, "mod_file_recorder: VideoH264Recorder::RunCloseShell() transcode to mp4 fail!, cmd:%s\n"
						, cmd);
		}
	}

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: RunCloseShell() end\n");
}

// 根据源文件路径获取SiteID
bool VideoH264Recorder::GetSiteIdWithFilePath(char* buffer, const char* srcPath)
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
bool VideoH264Recorder::GetStartTimeWithFilePath(char* buffer, const char* srcPath)
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
bool VideoH264Recorder::GetStandardTime(char* buffer, const char* srcTime)
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
bool VideoH264Recorder::GetUserIdWithFilePath(char* buffer, const char* srcPath)
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
bool VideoH264Recorder::GetFileNameWithoutExt(char* buffer, const char* filePath)
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
bool VideoH264Recorder::GetParamWithFileName(char* buffer, const char* fileName, int index)
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
bool VideoH264Recorder::BuildH264FilePath(const char* srcPath)
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
bool VideoH264Recorder::GetMp4FilePath(char* mp4Path, const char* dir)
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
bool VideoH264Recorder::WriteVideoData2File(const uint8_t* buffer, size_t bufferLen, size_t& writeLen)
{
	bool result = false;

	writeLen = 0;
	if (NULL != mpFile)
	{
		writeLen = fwrite(buffer, 1, bufferLen, mpFile);
		result = (writeLen == bufferLen);
	}
	return result;
}

// 释放视频录制队列里所有buffer及空闲队列
void VideoH264Recorder::DestroyVideoQueue()
{
	// 释放队列所有buffer
	if (NULL != mpVideoQueue) {
		switch_buffer_t* buffer = NULL;
		while (SWITCH_STATUS_SUCCESS == switch_queue_trypop(mpVideoQueue, (void**)&buffer))
		{
			RecycleFreeBuffer(buffer);
		}
	}
}

// 生成截图的h264生成路径
bool VideoH264Recorder::BuildPicH264FilePath(const char* srcPath, const char* dir)
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
bool VideoH264Recorder::BuildPicFilePath(const char* srcPath, const char* dir)
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
bool VideoH264Recorder::RenewPicBuffer(switch_buffer_t* buffer)
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
switch_buffer_t* VideoH264Recorder::PopPicBuffer()
{
	switch_buffer_t* buffer = NULL;

	switch_mutex_lock(mpPicMutex);
	buffer = mpPicBuffer;
	mpPicBuffer = NULL;
	switch_mutex_unlock(mpPicMutex);
	return buffer;
}

// 生成截图
bool VideoH264Recorder::BuildPicture(switch_buffer_t* buffer, uint8_t* dataBuffer, switch_size_t dataBufLen)
{
	bool result = false;

	result = BuildPicH264File(buffer, dataBuffer, dataBufLen);
	result = result && RunPictureShell();

	return result;
}

// 生成截图的H264文件
bool VideoH264Recorder::BuildPicH264File(switch_buffer_t* buffer, uint8_t* dataBuffer, switch_size_t dataBufLen)
{
	bool result = false;
	if (NULL != buffer
		&& switch_buffer_inuse(buffer) > 0
		&& switch_buffer_inuse(buffer) < dataBufLen)
	{
		// 打开文件
		FILE* pFile = fopen(mcPicRecPath, "w+b");
		if (NULL != pFile)
		{
			// 数据写入文件
			switch_size_t inuseSize = switch_buffer_inuse(buffer);
			switch_size_t procSize = switch_buffer_peek(buffer, dataBuffer, dataBufLen);
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
						, "mod_file_recorder: VideoH264Recorder::BuildPicH264File() fail! path:%s\n"
						, mcPicRecPath);
		}
	}

	// log
//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//		, "mod_file_recorder: VideoH264Recorder::BuildPicH264File() result:%d, path:%s\n"
//		, result, mcPicRecPath);

	return result;
}

// 执行生成截图shell
bool VideoH264Recorder::RunPictureShell()
{
	bool result = false;

	// build shell处理文件
	char cmd[MAX_PATH_LENGTH] = {0};
//	snprintf(cmd, sizeof(cmd), "%s %s %s"
//			, mcPicShell, mcPicRecPath, mcPicDstPath);
	snprintf(cmd, sizeof(cmd), "/usr/local/bin/ffmpeg -i %s -y -s 240x180 -vframes 1 %s"
			, mcPicRecPath, mcPicDstPath);

	// log
//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//				, "mod_file_recorder: VideoH264Recorder::RunPictureShell() transcode to jpeg start, cmd:%s\n"
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
//					, "mod_file_recorder: VideoH264Recorder::RunPictureShell() transcode to jpeg success, cmd:%s\n"
//					, cmd);
	}
	else {
		// error
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
					, "mod_file_recorder: VideoH264Recorder::RunPictureShell() transcode to jpeg fail!, cmd:%s\n"
					, cmd);
	}

	return result;
}

bool VideoH264Recorder::SendCommand(const char* cmd) {
//	switch_log_printf(
//			SWITCH_CHANNEL_LOG,
//			SWITCH_LOG_DEBUG,
//			"mod_file_recorder: VideoH264Recorder::SendCommand() cmd : \"%s\"\n",
//			cmd
//			);
	return file_record_send_command_lua(cmd);
}

bool VideoH264Recorder::file_record_send_command(const char* cmd) {
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
				"VideoH264Recorder::file_record_send_command( "
				"[Send event Fail], "
				"this : %p, "
				"cmd : '%s' "
				") \n",
				this,
				cmd
				);
	}

	return status == SWITCH_STATUS_SUCCESS;
}

bool VideoH264Recorder::file_record_send_command_lua(const char* cmd) {
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
//			"mod_file_recorder::VideoH264Recorder::file_record_send_command_lua( "
//			"[LUA call], "
//			"this : %p, "
//			"path : %s "
//			") \n",
//			this,
//			path
//			);

	if( strlen(path) > 0 && switch_api_execute("lua", path, NULL, &stream) == SWITCH_STATUS_SUCCESS ) {
		bFlag = true;
//		switch_log_printf(
//				SWITCH_CHANNEL_LOG,
//				SWITCH_LOG_INFO,
//				"mod_file_recorder::VideoH264Recorder::file_record_send_command_lua( "
//				"[LUA call OK], "
//				"this : %p, "
//				"stream.data : '%s' "
//				") \n",
//				this,
//				stream.data
//				);
	} else {
		switch_log_printf(
				SWITCH_CHANNEL_LOG,
				SWITCH_LOG_ERROR,
				"mod_file_recorder::VideoH264Recorder::file_record_send_command_lua( "
				"[LUA call Fail], "
				"this : %p, "
				"stream.data : '%s' "
				") \n",
				this,
				stream.data
				);
	}

//	switch_safe_free(stream.data);

	return bFlag;
}

// 获取空闲buffer
switch_buffer_t* VideoH264Recorder::GetFreeBuffer()
{
	switch_buffer_t* buffer = NULL;
	if (NULL != mpFreeBufferQueue)
	{
		if (SWITCH_STATUS_SUCCESS != switch_queue_trypop(mpFreeBufferQueue, (void**)&buffer))
		{
			// 获取失败，尝试创建
			if (SWITCH_STATUS_SUCCESS != switch_buffer_create(mpMemoryPool, &buffer, H264_BUFFER_SIZE))
			{
				// 创建空闲buffer失败
				buffer = NULL;

				// error
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
							, "mod_file_recorder: VideoH264Recorder::GetFreeBuffer() get free buffer fail! mpFreeBufferQueue:%p, mpMemoryPool:%p\n"
							, mpFreeBufferQueue, mpMemoryPool);
			}
			else {
				// 创建空闲buffer成功，统计共生成多少个buffer
				miCreateBufferCount++;

//				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//							, "mod_file_recorder: VideoH264Recorder::GetFreeBuffer() miCreateBufferCount:%d, buffer:%p\n"
//							, miCreateBufferCount, buffer);
			}
		}
	}
	return buffer;
}

// 回收空闲buffer
void VideoH264Recorder::RecycleFreeBuffer(switch_buffer_t* buffer)
{
//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//			, "mod_file_recorder: VideoH264Recorder::RecycleFreeBuffer() start! buffer:%p, mpFreeBufferQueue:%p\n"
//			, buffer, mpFreeBufferQueue);

	if (NULL != buffer
		&& NULL != mpFreeBufferQueue)
	{
		// log
//		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//				, "mod_file_recorder: VideoH264Recorder::RecycleFreeBuffer() recycle free buffer success! buffer:%p\n"
//				, buffer);

		switch_buffer_zero(buffer);
		switch_queue_push(mpFreeBufferQueue, buffer);
	}
	else {
		// log
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
				, "mod_file_recorder: VideoH264Recorder::RecycleFreeBuffer() recycle free buffer error! buffer:%p, mpFreeBufferQueue:%p\n"
				, buffer, mpFreeBufferQueue);
	}
}

// 释放空闲队列里所有buffer及空闲队列
void VideoH264Recorder::DestroyFreeBufferQueue()
{
	// 释放队列里所有buffer
	if (NULL != mpFreeBufferQueue) {
		switch_buffer_t* buffer = NULL;
		while (SWITCH_STATUS_SUCCESS == switch_queue_trypop(mpFreeBufferQueue, (void**)&buffer))
		{
			switch_buffer_destroy(&buffer);
		}
	}
}

