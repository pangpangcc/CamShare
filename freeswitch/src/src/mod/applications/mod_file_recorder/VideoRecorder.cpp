/*
 * VideoRecorder.cpp
 *
 *  Created on: 2016-04-26
 *      Author: Samson.Fan
 * Description: 录制视频文件及监控截图
 */

#include "VideoRecorder.h"
#include "CommonFunc.h"

#define H264_BUFFER_SIZE (1024 * 128)

VideoRecorder::VideoRecorder()
{
//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "mod_file_recorder: VideoRecorder()\n");

	ResetParam();
}

VideoRecorder::~VideoRecorder()
{
//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "mod_file_recorder: ~VideoRecorder()\n");

//	StopRecord();
}

// 重置参数
void VideoRecorder::ResetParam()
{
	mbRunning = false;
	mpMutex = NULL;

	memset(mcH264Path, 0, sizeof(mcH264Path));
	memset(mcMp4Dir, 0, sizeof(mcMp4Dir));
	memset(mcCloseShell, 0, sizeof(mcCloseShell));

	mpFile = NULL;
	mpHandle = NULL;

	mpVideoQueue = NULL;
	mpVideoThread = NULL;
	mpFreeBufferQueue = NULL;

	mpMemoryPool = NULL;
	mbNaluStart = SWITCH_FALSE;
	mpNaluBuffer = NULL;

	memset(mcPicH264Path, 0, sizeof(mcPicH264Path));
	memset(mcPicPath, 0, sizeof(mcPicPath));
	memset(mcPicShell, 0, sizeof(mcPicShell));
	miPicInterval = 0;

	mpPicMutex = NULL;
	mpPicThread = NULL;
	mpPicBuffer = NULL;
}

// 开始录制
bool VideoRecorder::StartRecord(switch_file_handle_t *handle
			, const char *path, const char* mp4dir, const char* closeshell
			, const char* pich264dir, const char* picdir, const char* picshell, int picinterval)
{
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
			, "mod_file_recorder: StartRecord() handle:%p, memory_pool:%p"
			  ", path:%s, mp4dir:%s, closeshell:%s"
			  ", pich264dir:%s, picdir:%s, picshell:%s, picinterval:%d\n"
			, handle
			, (NULL!= handle ? handle->memory_pool : NULL)
			, path, mp4dir, closeshell
			, pich264dir, picdir, picshell, picinterval);

	// 确保不是正在运行，且目录没有问题
	if (!mbRunning
		&& BuildH264FilePath(path)
		&& IsDirExist(mp4dir))
	{
		bool success = true;

		strcpy(mcCloseShell, closeshell);
		strcpy(mcMp4Dir, mp4dir);

//		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//				, "mod_file_recorder: StartRecord() mcH264Path:%s, mcMp4Dir:%s, mcCloseShell:%s\n"
//				, mcH264Path, mcMp4Dir, mcCloseShell);

		// 创建内存池
		if (success) {
			success = success
					&& (SWITCH_STATUS_SUCCESS == switch_core_new_memory_pool(&mpMemoryPool));

//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//							, "mod_file_recorder: StartRecord() mpMemoryPool:%p\n"
//							, mpMemoryPool);
		}

		// 记录句柄
		if (success) {
			mpHandle = handle;
		}

		// 打开文件
		if (success) {
			mpFile = fopen(mcH264Path, "w+b");
			success = success && (NULL != mpFile);

			if (!success) {
				// error
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
					, "mod_file_recorder: StartRecord() open file fail, handle:%p, path:%s\n"
					, handle, path);
			}

//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//											, "mod_file_recorder: StartRecord() mpFile:%p, success:%d\n"
//											, mpFile, success);
		}

		// 打开文件成功
		if(success)
		{
			// 标记为已经打开文件
			mbRunning = true;

//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//							, "mod_file_recorder: StartRecord() load video recorder source start\n");

			// 创建空闲buffer队列
			success = success
					&& (SWITCH_STATUS_SUCCESS
							== switch_queue_create(&mpFreeBufferQueue, SWITCH_CORE_QUEUE_LEN, mpMemoryPool));

//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//							, "mod_file_recorder: StartRecord() load free buffer queue, success:%d\n"
//							, success);

			// 创建Nalu缓冲
			success = success && RenewNaluBuffer();

//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//							, "mod_file_recorder: StartRecord() renew nalu buffer, success:%d\n"
//							, success);

			// 创建缓存队列
			success = success
					&& (SWITCH_STATUS_SUCCESS
							== switch_queue_create(&mpVideoQueue, SWITCH_CORE_QUEUE_LEN, mpMemoryPool));
			success = success
					&& (SWITCH_STATUS_SUCCESS
							== switch_mutex_init(&mpMutex, SWITCH_MUTEX_NESTED, mpMemoryPool));

//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//							, "mod_file_recorder: StartRecord() load video queue, success:%d\n"
//							, success);

			// 创建录制视频线程
			switch_threadattr_t *thd_video_attr = NULL;
			success = success
					&& (SWITCH_STATUS_SUCCESS
							== switch_threadattr_create(&thd_video_attr, mpMemoryPool));
			success = success
					&& (SWITCH_STATUS_SUCCESS
							== switch_threadattr_stacksize_set(thd_video_attr, SWITCH_THREAD_STACKSIZE));
			success = success
					&& (SWITCH_STATUS_SUCCESS
							== switch_thread_create(&mpVideoThread, thd_video_attr, VideoRecorder::RecordVideoFrame2FileThread, this, mpMemoryPool));

//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//							, "mod_file_recorder: StartRecord() load video thread, success:%d\n"
//							, success);

			// 截图处理
			if (success)
			{
				if (picinterval > 0
					&& BuildPicH264FilePath(path, pich264dir)
					&& BuildPicFilePath(path, picdir)
					&& strlen(picshell) > 0)
				{
					// 设置参数
					miPicInterval = picinterval;
					strcpy(mcPicShell, picshell);

//					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//									, "mod_file_recorder: StartRecord() load cut picture start, success:%d\n"
//									, success);

					// 创建锁
					success = success
							&& (SWITCH_STATUS_SUCCESS
									== switch_mutex_init(&mpPicMutex, SWITCH_MUTEX_NESTED, mpMemoryPool));

//					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//									, "mod_file_recorder: StartRecord() load cut picture mutex, success:%d\n"
//									, success);

					// 创建生成监控图片线程
					switch_threadattr_t *thd_pic_attr = NULL;
					success = success
							&& (SWITCH_STATUS_SUCCESS
									== switch_threadattr_create(&thd_pic_attr, mpMemoryPool));
					success = success
							&& (SWITCH_STATUS_SUCCESS
									== switch_threadattr_stacksize_set(thd_pic_attr, SWITCH_THREAD_STACKSIZE));
					success = success
							&& (SWITCH_STATUS_SUCCESS
									== switch_thread_create(&mpPicThread, thd_pic_attr, VideoRecorder::RecordPicture2FileThread, this, mpMemoryPool));

//					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//									, "mod_file_recorder: StartRecord() load cut picture thread, success:%d\n"
//									, success);
				}
				else
				{
					// log
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
						, "mod_file_recorder: StartRecord() handle:%p, picinterval:%d, pich264dir:%s, picdir:%s"
						  ", picshell:%s, path:%s\n"
						, handle, picinterval, pich264dir, picdir
						, picshell, path);

					if (!IsDirExist(pich264dir)) {
						// 目录不存在
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
							, "mod_file_recorder: StartRecord() pich264dir:%s not exist\n"
							, pich264dir);
					}

					if (!IsDirExist(picdir)) {
						// 目录不存在
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
							, "mod_file_recorder: StartRecord() picdir:%s not exist\n"
							, picdir);
					}
				}
			}

			// 更新打开成功标记
			mbRunning = success;
		}
	}

	// 处理不成功
	if (!mbRunning)
	{
		// log
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
				, "mod_file_recorder: StartRecord() fail! path:%s, mp4dir:%s, handle:%p"
				  ", mpFile:%p, mpMemoryPool:%p, mpVideoQueue:%p, mpMutex:%p, mpVideoThread:%p\n"
				, path, mp4dir, handle
				, mpFile, mpMemoryPool, mpVideoQueue, mpMutex, mpVideoThread);

		if (!IsDirExist(mp4dir)) {
			// 目录不存在
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
				, "mod_file_recorder: StartRecord() mp4dir:%s not exist\n"
				, mp4dir);
		}

		// 释放资源
		StopRecord();
	}

	// 打log
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
			, "mod_file_recorder: StartRecord() finish, handle:%p, mbRunning:%d, path:%s\n"
			, handle, mbRunning, path);

	return mbRunning;
}

// 停止录制
void VideoRecorder::StopRecord()
{
	// log for test
//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//			, "mod_file_recorder: StopRecord() start, handle:%p, mpMemoryPool:%p, mpFile:%p, mbRunning:%d\n"
//			, mpHandle, mpMemoryPool, mpFile, mbRunning);

	// 用于判断是否重置前状态
	bool isOldRunning = false;

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//				, "mod_file_recorder: StopRecord() set isOldRunning\n");

	// 标记为需要关闭文件
	if (NULL != mpMutex) {
		switch_mutex_lock(mpMutex);
	}
	isOldRunning = mbRunning;
	mbRunning = false;
	if (NULL != mpMutex) {
		switch_mutex_unlock(mpMutex);
	}

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: StopRecord() unlock\n");

	// 停止录制视频线程
	if(NULL != mpVideoThread)
	{
		switch_status_t st;
		switch_thread_join(&st, mpVideoThread);
		mpVideoThread = NULL;
	}

	// 停止截图线程
	if(NULL != mpPicThread)
	{
		switch_status_t st;
		switch_thread_join(&st, mpPicThread);
		mpPicThread = NULL;
	}

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: StopRecord() over thread\n");

	// 关闭文件
	if(NULL != mpFile)
	{
		fclose(mpFile);
		mpFile = NULL;
	}

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: StopRecord() close file\n");

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
//					, "mod_file_recorder: StopRecord() destroy mpNaluBuffer\n");


	// 释放空闲buffer队列
	DestroyFreeBufferQueue();
	// 释放内存池
	if (NULL != mpMemoryPool)
	{
		switch_core_destroy_memory_pool(&mpMemoryPool);
	}

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//				, "mod_file_recorder: StopRecord() finish, isOldRunning:%d, handle:%p, path:%s, mcCloseShell:%s\n"
//				, isOldRunning, mpHandle, mcH264Path, mcCloseShell);

	// 之前状态为running，执行关闭shell
	if (isOldRunning)
	{
		// log for test
//		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//				, "mod_file_recorder: StopRecord() runing close shell, handle:%p\n"
//				, mpHandle);
		RunCloseShell();
	}

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
			, "mod_file_recorder: StopRecord() finish, handle:%p\n"
			, mpHandle);

	// 重置参数
	ResetParam();
}

// 录制视频frame
bool VideoRecorder::RecordVideoFrame(switch_frame_t *frame)
{
	bool bFlag = true;

	// 加锁，防止线程已经停止，但本函数只处理一半，仍向队列插入数据
	switch_mutex_lock(mpMutex);

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: RecordVideoFrame() start handle:%p\n"
//					, mpHandle);

	if (mbRunning)
	{
//		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//				, "mod_file_recorder: RecordVideoFrame() handle:%p, datalen:%d\n"
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
//					, "mod_file_recorder: RecordVideoFrame() handle:%p, SWITCH_STATUS_RESTART, mcH264Path:%s\n"
//					, mpHandle, mcH264Path);

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
//					, "mod_file_recorder: RecordVideoFrame() push queue, handle:%p, mcH264Path:%s\n"
//					, mpHandle, mcH264Path);
		}
	}
	else {
		// error
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
						, "mod_file_recorder: RecordVideoFrame() stoped, mbRunning:%d\n"
						, mbRunning);

		bFlag = false;
	}

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: RecordVideoFrame() end handle:%p\n"
//					, mpHandle);

	switch_mutex_unlock(mpMutex);

	return bFlag;
}

// 是否正在运行
bool VideoRecorder::IsRunning()
{
	return mbRunning;
}

// 录制视频线程
void* SWITCH_THREAD_FUNC VideoRecorder::RecordVideoFrame2FileThread(switch_thread_t* thread, void* obj)
{
	VideoRecorder *recorder = (VideoRecorder*) obj;
	recorder->RecordVideoFrame2FileProc();
	return NULL;
}

// 录制视频线程处理
void VideoRecorder::RecordVideoFrame2FileProc()
{
	void* pop = NULL;
	switch_status_t status;
	switch_interval_time_t timeout = 1000 * 1000;

	// log for test
//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//			, "mod_file_recorder: RecordVideoFrame2FileProc() start, handle:%p\n"
//			, mpHandle);

	// 缓存定义
	const size_t dataBufferSize = H264_BUFFER_SIZE;
	size_t dataBufferLen = 0;
	uint8_t* dataBuffer = (uint8_t*)switch_core_alloc(mpMemoryPool, dataBufferSize);
	// 当前buffer在缓存的偏移位置
	size_t currBufferOffset = 0;

	while (mbRunning)
	{
//		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//				, "mod_file_recorder: RecordVideoFrame2FileProc() pop start\n");

		// 获取缓存视频frame
		status = switch_queue_pop_timeout(mpVideoQueue, &pop, timeout);

		// log for test
//		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//				, "mod_file_recorder: RecordVideoFrame2FileProc() pop, status:%d, SWITCH_STATUS_TIMEOUT:%d\n"
//				, status, SWITCH_STATUS_TIMEOUT);

		if (status == SWITCH_STATUS_SUCCESS)
		{
			bool bFreak = false;
			switch_buffer_t* buffer = (switch_buffer_t*)pop;
			switch_size_t inuseSize = switch_buffer_inuse(buffer);
			if (inuseSize > 0)
			{
				if (dataBufferLen + inuseSize >= dataBufferSize) {
					// 缓存不足写入文件
					size_t writeLen = 0;
					if (WriteVideoData2File(dataBuffer, dataBufferLen, writeLen)) {
						// 写入成功
						dataBufferLen = 0;
					}
					else {
						// 写入不成功
						if (writeLen > 0) {
							// buffer数据前移，并减少buffer的已用数据长度
							memcpy(dataBuffer, dataBuffer + writeLen, dataBufferLen - writeLen);
							dataBufferLen -= writeLen;
						}

						// error
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
										, "mod_file_recorder: RecordVideoFrame2FileProc() write file fail, handle:%p, dataBufferLen:%d, writeLen:%d, mcH264Path:%s\n"
										, mpHandle, dataBufferLen, writeLen, mcH264Path);
					}
				}

				// 偏移至本帧数据写入的位置
				currBufferOffset = dataBufferLen;

				// 视频数据写入缓存
				if (dataBufferLen + inuseSize < dataBufferSize) {
					switch_size_t procSize = switch_buffer_peek(buffer, dataBuffer + dataBufferLen, dataBufferSize - dataBufferLen);
					if (procSize == inuseSize) {
						dataBufferLen += procSize;
					}
					else {
						// 写入缓存失败
						bFreak = true;
					}
				}
			}

			// log for test
//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: RecordVideoFrame2FileProc() write finish, inuseSize:%d, bFreak:%d, mcH264Path:%s\n"
//					, inuseSize, bFreak, mcH264Path);

			bool isDestroyBuffer = true;
			if (!bFreak)
			{
				// handle i-frame
				bool isIFrame = IsIFrame(dataBuffer + currBufferOffset, inuseSize);
				if (isIFrame)
				{
					// 传到截图buffer & 不释放
					if (NULL != mpPicThread) {
						RenewPicBuffer(buffer);
						isDestroyBuffer = false;
					}
				}

//				if (!isDestroyBuffer)
//				{
//					// log for test
//					uint8_t* frameData = dataBuffer + currBufferOffset;
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
//						, "mod_file_recorder: RecordVideoFrame2FileProc()"
//						  " inuseSize:%d, bFreak:%d, isDestroyBuffer:%d, isIFrame:%d, mpPicBuffer:%p\n"
//						  "%s"
//						, inuseSize, bFreak, isDestroyBuffer, isIFrame, mpPicBuffer
//						, logBuffer);
//				}
			}

			if (isDestroyBuffer)
			{
				// 回收buffer
				RecycleFreeBuffer(buffer);
			}

			// 读buffer数据失败
			if( bFreak ) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
						, "mod_file_recorder: read buffer fail, handle:%p, inuse:%d, dataBufferLen:%d, dataBufferSize:%d, mcH264Path:%s\n"
						, mpHandle, inuseSize, dataBufferLen, dataBufferSize, mcH264Path);
				break;
			}

		}

//		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//				, "mod_file_recorder: RecordVideoFrame2FileProc() pop end\n");
	}

	// 把剩下的缓存写入文件
	if (dataBufferLen > 0)
	{
		size_t writeLen = 0;
		if (!WriteVideoData2File(dataBuffer, dataBufferLen, writeLen)) {
			// 写入文件不成功
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
							, "mod_file_recorder: RecordVideoFrame2FileProc() write file fail, handle:%p, dataBufferLen:%d, writeLen:%d\n"
							, mpHandle, dataBufferLen, writeLen);
		}
	}

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//			, "mod_file_recorder: RecordVideoFrame2FileProc() stop, handle:%p\n"
//			, mpHandle);
}

// 监控截图线程
void* SWITCH_THREAD_FUNC VideoRecorder::RecordPicture2FileThread(switch_thread_t* thread, void* obj)
{
	VideoRecorder *recorder = (VideoRecorder*) obj;
	recorder->RecordPicture2FileProc();
	return NULL;
}

// 监制截图线程处理
void VideoRecorder::RecordPicture2FileProc()
{
//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//			, "mod_file_recorder: RecordPicture2FileProc() start, handle:%p\n"
//			, mpHandle);

	// 创建写文件buffer
	const size_t dataBufLen = H264_BUFFER_SIZE;
	uint8_t* dataBuffer = (uint8_t*)switch_core_alloc(mpMemoryPool, H264_BUFFER_SIZE);

	while (mbRunning)
	{
		// 生成下一个周期的处理时间
		long long endTime = getCurrentTime() + miPicInterval;

		// log for test
//		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: RecordPicture2FileProc() BuildPicture start\n");

		// 生成图片
		bool bBuildPicResult = false;
		switch_mutex_lock(mpPicMutex);
		if (NULL != mpPicBuffer
			&& switch_buffer_inuse(mpPicBuffer) > 0)
		{
			bBuildPicResult = BuildPicture(mpPicBuffer, dataBuffer, dataBufLen);
		}
		switch_mutex_unlock(mpPicMutex);

		// log for test
//		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: RecordPicture2FileProc() BuildPicture finish, result:%d\n"
//					, bBuildPicResult);

		// 等下一个周期
		while (mbRunning && getCurrentTime() < endTime)
		{
			usleep(200 * 1000);
		}
	}

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//			, "mod_file_recorder: RecordPicture2FileProc() stop, handle:%p\n"
//			, mpHandle);
}

// 判断是否i帧
bool VideoRecorder::IsIFrame(const uint8_t* data, switch_size_t inuse)
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
			nalTypeCount = data[currNalOffset + 4] & nalIdcBit;
			nalTypeCount = nalTypeCount >> 5;
		}
		else {
//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//				, "mod_file_recorder: IsIFrame() memcmp() false\n");
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
//						, "mod_file_recorder: IsIFrame() nalType:%d, isIFrame:%d\n"
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
//						, "mod_file_recorder: IsIFrame() nalType:%d, nalHeadSize:%d, nalSpsSize:%d, currNalOffset:%d\n"
//						, nalType, nalHeadSize, nalSpsSize, currNalOffset);
				}
				else if (8 == nalType) {
					// PPS (offset PPS size + nal head size)
					currNalOffset += nalHeadSize + nalPpsSize;

//					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//						, "mod_file_recorder: IsIFrame() nalType:%d, nalHeadSize:%d, nalPpsSize:%d, currNalOffset:%d\n"
//						, nalType, nalHeadSize, nalPpsSize, currNalOffset);
				}
				else {
//					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//						, "mod_file_recorder: IsIFrame() nalType:%d, currNalOffset:%d\n"
//						, nalType, currNalOffset);
				}
			}
			else {
//				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: IsIFrame() memcmp() false, currNalOffset:%d, nalHeadSize:%d\n"
//					, currNalOffset, nalHeadSize);
			}
		}
	}
	else {
//		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//			, "mod_file_recorder: IsIFrame() inuse > nalHeadSize false, inuse:%d, nalHeadSize:%d\n"
//			, inuse, nalHeadSize);
	}

	return isIFrame;
}

switch_status_t VideoRecorder::buffer_h264_nalu(switch_frame_t *frame, switch_buffer_t* nalu_buffer, switch_bool_t& nalu_28_start)
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
bool VideoRecorder::RenewNaluBuffer()
{
	bool result = false;
	if (IsRunning()
		&& NULL != mpMemoryPool)
	{
		mpNaluBuffer = GetFreeBuffer();
		result = (NULL != mpNaluBuffer);
	}
	return result;
}

// 执行关闭shell
void VideoRecorder::RunCloseShell()
{
//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: RunCloseShell() start\n");
	if (strlen(mcCloseShell) > 0)
	{
		// get mp4Path
		char mp4Path[MAX_PATH_LENGTH] = {0};
		GetMp4FilePath(mp4Path, mcMp4Dir);

		// get userId
		char userId[MAX_PATH_LENGTH] = {0};
		GetUserIdWithFilePath(userId, mcH264Path);

		// get siteId
		char siteId[MAX_PATH_LENGTH] = {0};
		GetSiteIdWithFilePath(siteId, mcH264Path);

		// get startTime
		char startTime[MAX_PATH_LENGTH] = {0};
		GetStartTimeWithFilePath(startTime, mcH264Path);
		char startStandardTime[32] = {0};
		GetStandardTime(startStandardTime, startTime);

		// print param
//		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//						, "mod_file_recorder: run close shell, 0:%s, 1:%s, 2:%s, 3:%s, 4:%s, 5:%s, 6:%s\n"
//						, mcCloseShell, mcH264Path, mp4Path, mcPicPath, userId, siteId, startStandardTime);

		// build shell处理文件
		// shell h264Path mp4Path jpgPath userId siteId startTime
		char cmd[MAX_PATH_LENGTH] = {0};
		snprintf(cmd, sizeof(cmd), "%s '%s' '%s' '%s' '%s' '%s' '%s' '%s'"
				, mcCloseShell, mcH264Path, mp4Path, mcPicPath, mcPicH264Path
				, userId, siteId, startStandardTime);

		// log for test
//		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: transcode to mp4 start, cmd:%s\n"
//					, cmd);

		// run shell
		int result = system(cmd);

		// log
		if (result >= 0) {
			// success
//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//						, "mod_file_recorder: transcode to mp4 success, result:%d, cmd:%s\n"
//						, result, cmd);
		}
		else {
			// error
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
						, "mod_file_recorder: transcode to mp4 fail! result:%d, cmd:%s\n"
						, result, cmd);
		}
	}

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: RunCloseShell() end\n");
}

// 根据源文件路径获取SiteID
bool VideoRecorder::GetSiteIdWithFilePath(char* buffer, const char* srcPath)
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
bool VideoRecorder::GetStartTimeWithFilePath(char* buffer, const char* srcPath)
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
bool VideoRecorder::GetStandardTime(char* buffer, const char* srcTime)
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
bool VideoRecorder::GetUserIdWithFilePath(char* buffer, const char* srcPath)
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
bool VideoRecorder::GetFileNameWithoutExt(char* buffer, const char* filePath)
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
bool VideoRecorder::GetParamWithFileName(char* buffer, const char* fileName, int index)
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
bool VideoRecorder::BuildH264FilePath(const char* srcPath)
{
	bool result = false;
	if (NULL != srcPath && srcPath[0] != '\0')
	{
		strcat(mcH264Path, srcPath);
		result = true;
	}
	return result;
}

// 获取mp4文件路径
bool VideoRecorder::GetMp4FilePath(char* mp4Path, const char* dir)
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
bool VideoRecorder::WriteVideoData2File(const uint8_t* buffer, size_t bufferLen, size_t& writeLen)
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
void VideoRecorder::DestroyVideoQueue()
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
bool VideoRecorder::BuildPicH264FilePath(const char* srcPath, const char* dir)
{
	bool result = false;
	if (NULL != srcPath && srcPath[0] != '\0'
		&& NULL != dir && dir[0] != '\0')
	{
		int dirLen = strlen(dir);
		if (dirLen > 0)
		{
			// copy dir path
			strcpy(mcPicH264Path, dir);
			if (dir[dirLen-1] != '/'
				&& dir[dirLen-1] != '\\')
			{
				strcat(mcPicH264Path, "/");
			}

			// copy file user id
			char userId[MAX_PATH_LENGTH] = {0};
			if (GetUserIdWithFilePath(userId, srcPath))
			{
				strcat(mcPicH264Path, userId);
				strcat(mcPicH264Path, ".h264");

				result = true;
			}
		}
	}
	return result;
}

// 生成截图图片路径
bool VideoRecorder::BuildPicFilePath(const char* srcPath, const char* dir)
{
	bool result = false;
	if (NULL != srcPath && srcPath[0] != '\0'
		&& NULL != dir && dir[0] != '\0')
	{
		int dirLen = strlen(dir);
		if (dirLen > 0)
		{
			// copy dir path
			strcpy(mcPicPath, dir);
			if (dir[dirLen-1] != '/'
				&& dir[dirLen-1] != '\\')
			{
				strcat(mcPicPath, "/");
			}

			// copy file user id
			char userId[MAX_PATH_LENGTH] = {0};
			if (GetUserIdWithFilePath(userId, srcPath))
			{
				strcat(mcPicPath, userId);
				strcat(mcPicPath, ".jpg");

				result = true;
			}
		}
	}
	return result;
}

// 重建pic缓冲
bool VideoRecorder::RenewPicBuffer(switch_buffer_t* buffer)
{
	switch_mutex_lock(mpPicMutex);
	if (NULL != mpPicBuffer) {
		RecycleFreeBuffer(mpPicBuffer);
		mpPicBuffer = NULL;
	}

	mpPicBuffer = buffer;
	switch_mutex_unlock(mpPicMutex);
	return true;
}

// 生成截图
bool VideoRecorder::BuildPicture(switch_buffer_t* buffer, uint8_t* dataBuffer, switch_size_t dataBufLen)
{
	bool result = false;

	result = BuildPicH264File(buffer, dataBuffer, dataBufLen);
	result = result && RunPictureShell();

	return result;
}

// 生成截图的H264文件
bool VideoRecorder::BuildPicH264File(switch_buffer_t* buffer, uint8_t* dataBuffer, switch_size_t dataBufLen)
{
	bool result = false;
	if (NULL != buffer
		&& switch_buffer_inuse(buffer) > 0
		&& switch_buffer_inuse(buffer) < dataBufLen)
	{
		// 打开文件
		FILE* pFile = fopen(mcPicH264Path, "w+b");
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
						, "mod_file_recorder: BuildPicH264File fail! path:%s\n"
						, mcPicH264Path);
		}
	}

	// error
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
		, "mod_file_recorder: BuildPicH264File() result:%d, path:%s\n"
		, result, mcPicH264Path);

	return result;
}

// 执行生成截图shell
bool VideoRecorder::RunPictureShell()
{
	bool result = false;

	// build shell处理文件
	char cmd[MAX_PATH_LENGTH] = {0};
	snprintf(cmd, sizeof(cmd), "%s %s %s"
			, mcPicShell, mcPicH264Path, mcPicPath);

	// log
//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//				, "mod_file_recorder: transcode to jpeg start, cmd:%s\n"
//				, cmd);

	// run shell
	int code = system(cmd);
	result = (code >= 0);

	// log
	if (result) {
		// log for test
//		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: transcode to jpeg success, code:%d, cmd:%s\n"
//					, code, cmd);
	}
	else {
		// error
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
					, "mod_file_recorder: transcode to jpeg fail! code:%d, cmd:%s\n"
					, code, cmd);
	}

	return result;
}

// 获取空闲buffer
switch_buffer_t* VideoRecorder::GetFreeBuffer()
{
	switch_buffer_t* buffer = NULL;
	if (NULL != mpFreeBufferQueue)
	{
		if (SWITCH_STATUS_SUCCESS != switch_queue_trypop(mpFreeBufferQueue, (void**)&buffer)
			&& SWITCH_STATUS_SUCCESS != switch_buffer_create(mpMemoryPool, &buffer, H264_BUFFER_SIZE))
		{
			// 获取及创建空闲buffer失败
			buffer = NULL;

			// error
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
						, "mod_file_recorder: get free buffer fail! mpFreeBufferQueue:%p, mpMemoryPool:%p\n"
						, mpFreeBufferQueue, mpMemoryPool);
		}
	}
	return buffer;
}

// 回收空闲buffer
void VideoRecorder::RecycleFreeBuffer(switch_buffer_t* buffer)
{
	if (NULL != buffer
		&& NULL != mpFreeBufferQueue)
	{
		switch_buffer_zero(buffer);
		switch_queue_push(mpFreeBufferQueue, buffer);
	}
	else {
		// log
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
				, "mod_file_recorder: recycle free buffer error! buffer:%p, mpFreeBufferQueue:%p\n"
				, buffer, mpFreeBufferQueue);
	}
}

// 释放空闲队列里所有buffer及空闲队列
void VideoRecorder::DestroyFreeBufferQueue()
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

