/*
 * VideoRecordManager.cpp
 *
 *  Created on: 2016-08-16
 *      Author: Samson.Fan
 * Description: 录制视频管理器
 */

#include "VideoRecordManager.h"
#include "CommonFunc.h"

VideoRecordManager::VideoRecordManager()
{
	// 初始化
	m_pool = NULL;
	m_isInit = false;
	m_isInitConfigure = false;

	// 配置
	memset(m_videoMp4Dir, 0, sizeof(m_videoMp4Dir));
	memset(m_videoCloseShell, 0, sizeof(m_videoCloseShell));
	m_videoThreadCount = 4;
	m_videoCloseThreadCount = 1;
	memset(m_picH264Dir, 0, sizeof(m_picH264Dir));
	memset(m_picDir, 0, sizeof(m_picDir));
	memset(m_picShell, 0, sizeof(m_picShell));
	m_picInterval = 1000;
	m_picThreadCount = 4;

	// VideoRecorder管理
	m_queueFree = NULL;
	m_recycleThread = NULL;
	m_queueToRecycle = NULL;
	m_isStartRecycle = false;
	m_videoRecorderCount = 0;
	m_videoRecorderCountMax = 0;

	// 视频处理
	m_videoThread = NULL;
	m_queueVideo = NULL;
	m_isStartVideo = false;

	// 监控截图处理
	m_picThread = NULL;
	m_queuePicture = NULL;
	m_isStartPicture = false;
}

VideoRecordManager::~VideoRecordManager()
{
	StopVideo();
	StopPicture();
	StopRecycle();
}

// 初始化
bool VideoRecordManager::Init(switch_memory_pool_t* pool)
{
	bool result = false;
	if (NULL != pool) {
		// 初始化参数
		m_pool = pool;

		// 初始化
		switch_queue_create(&m_queueFree, SWITCH_CORE_QUEUE_LEN, m_pool);
		switch_queue_create(&m_queueToRecycle, SWITCH_CORE_QUEUE_LEN, m_pool);
		switch_queue_create(&m_queueVideo, SWITCH_CORE_QUEUE_LEN, m_pool);
		switch_queue_create(&m_queuePicture, SWITCH_CORE_QUEUE_LEN, m_pool);

		switch_mutex_init(&mpVideoRecorderCountMutex, SWITCH_MUTEX_NESTED, m_pool);

		// 设置结果
		m_isInit = true;
		result = m_isInit;
	}
	return result;
}

// 初始化配置
bool VideoRecordManager::InitConfigure(const char* videoMp4Dir, const char* videoCloseShell, int videoThreadCount, int videoCloseThreadCount
		, const char* picH264Dir, const char* picDir, const char* picShell, int picInterval, int picThreadCount, int videoRecorderCountMax)
{
	bool result = false;

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//			, "mod_file_recorder: VideoRecordManager::InitConfigure() start mp4dir:%s, closeshell:%s, videothread:%d"
//			  ", picthread:%d, picinterval:%d, pich264dir:%s, picdir:%s, picshell:%s"
//			  ", result:%d\n"
//			, videoMp4Dir, videoCloseShell, videoThreadCount
//			, picThreadCount, picInterval, picH264Dir, picDir, picShell
//			, result);

	if (NULL != videoMp4Dir && videoMp4Dir[0] != '\0'
		&& NULL != videoCloseShell && videoCloseShell[0] != '\0'
		&& videoThreadCount > 0
		&& videoCloseThreadCount > 0)
	{
		// 设置结果
		m_isInitConfigure = true;
		result = m_isInitConfigure;

		// copy video configure
		strcpy(m_videoMp4Dir, videoMp4Dir);
		strcpy(m_videoCloseShell, videoCloseShell);
		m_videoThreadCount = videoThreadCount;
		m_videoCloseThreadCount = videoCloseThreadCount;
		m_videoRecorderCountMax = videoRecorderCountMax;
		// start recycle handle
		StartRecycle();

		// start video handle
		StartVideo();

		// copy picture configure
		if (NULL != picH264Dir && picH264Dir[0] != '\0'
			&& NULL != picDir && picDir[0] != '\0'
			&& NULL != picShell && picShell[0] != '\0'
			&& picInterval > 0
			&& picThreadCount > 0)
		{
			// copy picture configure
			strcpy(m_picH264Dir, picH264Dir);
			strcpy(m_picDir, picDir);
			strcpy(m_picShell, picShell);
			m_picInterval = picInterval;
			m_picThreadCount = picThreadCount;

			// start picture handle
			StartPicture();
		}
		else {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
					, "mod_file_recorder: VideoRecordManager::InitConfigure() picture fail, mp4dir:%s, closeshell:%s, videothread:%d"
					  ", picthread:%d, picinterval:%d, pich264dir:%s, picdir:%s, picshell:%s"
					  ", result:%d\n"
					, m_videoMp4Dir, m_videoCloseShell, m_videoThreadCount
					, m_picThreadCount, m_picInterval, m_picH264Dir, m_picDir, m_picShell
					, result);

			// stop picture handle
			StopPicture();
		}
	}
	else {
		// stop video handle
		StopVideo();
	}

	if (!result) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
				, "mod_file_recorder: VideoRecordManager::InitConfigure() fail, mp4dir:%s, closeshell:%s, videothread:%d"
				  ", picthread:%d, picinterval:%d, pich264dir:%s, picdir:%s, picshell:%s"
				  ", result:%d\n"
				, m_videoMp4Dir, m_videoCloseShell, m_videoThreadCount
				, m_picThreadCount, m_picInterval, m_picH264Dir, m_picDir, m_picShell
				, result);
	}

	return result;
}

// 启动视频录制
void VideoRecordManager::StartVideo()
{
//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoRecordManager::StartVideo() start\n");

	if (!m_isStartVideo
		&& m_isInit && m_isInitConfigure)
	{
		// 初始化启动参数
		m_isStartVideo = true;

		// 初始化线程参数
		switch_threadattr_t *thd_attr = NULL;
		switch_threadattr_create(&thd_attr, m_pool);
		switch_threadattr_detach_set(thd_attr, 1);
		switch_threadattr_stacksize_set(thd_attr, SWITCH_THREAD_STACKSIZE);
		switch_threadattr_priority_set(thd_attr, SWITCH_PRI_NORMAL);

		// 启动视频处理线程
		m_videoThread = (switch_thread_t**)switch_core_alloc(m_pool, m_videoThreadCount * sizeof(switch_thread_t*));
		int i = 0;
		for (i = 0; i < m_videoThreadCount; i++)
		{
			switch_thread_create(&m_videoThread[i], thd_attr, RecordVideoFrame2FileThread, this, m_pool);
		}
	}

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoRecordManager::StartVideo() end, start:%d\n"
//					, m_isStartVideo);
}

// 停止视频录制
void VideoRecordManager::StopVideo()
{
//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoRecordManager::StopVideo() start\n");

	if (m_isStartVideo)
	{
		// 设置停止
		m_isStartVideo = false;

		// 等待视频处理线程结束
		if (NULL != m_videoThread) {
			int i;
			for (i = 0; i < m_videoThreadCount; i++) {
				if (NULL != m_videoThread[i]) {
					switch_status_t st;
					switch_thread_join(&st, m_videoThread[i]);
				}
			}
		}
	}

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoRecordManager::StopVideo() end\n");
}

// 启动监控截图处理
void VideoRecordManager::StartPicture()
{
//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoRecordManager::StartPicture() start\n");

	if (!m_isStartPicture
		&& m_isInit && m_isInitConfigure)
	{
		// 初始化启动参数
		m_isStartPicture = true;

		// 初始化线程参数
		switch_threadattr_t *thd_attr = NULL;
		switch_threadattr_create(&thd_attr, m_pool);
		switch_threadattr_detach_set(thd_attr, 1);
		switch_threadattr_stacksize_set(thd_attr, SWITCH_THREAD_STACKSIZE);
		switch_threadattr_priority_set(thd_attr, SWITCH_PRI_LOW);

		// 启动监控截图处理线程
		m_picThread = (switch_thread_t**)switch_core_alloc(m_pool, m_picThreadCount * sizeof(switch_thread_t*));
		int i = 0;
		for (i = 0; i < m_picThreadCount; i++)
		{
			switch_thread_create(&m_picThread[i], thd_attr, RecordPicture2FileThread, this, m_pool);
		}
	}

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoRecordManager::StartPicture() end, start:%d\n"
//					, m_isStartPicture);
}

// 停止监控截图处理
void VideoRecordManager::StopPicture()
{
//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoRecordManager::StopPicture() start\n");

	if (m_isStartPicture)
	{
		// 设置停止
		m_isStartPicture = false;

		// 等待监控截图处理线程结束
		if (NULL != m_picThread) {
			int i;
			for (i = 0; i < m_picThreadCount; i++) {
				if (NULL != m_picThread[i]) {
					switch_status_t st;
					switch_thread_join(&st, m_picThread[i]);
				}
			}
		}
	}

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoRecordManager::StopPicture() end\n");
}

// 开始录制
bool VideoRecordManager::StartRecord(switch_file_handle_t *handle, const char *path)
{
	bool result = false;

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoRecordManager::StartRecord() start handle:%p, path:%s, start:%d\n"
//					, handle, path, m_isStartVideo);

	if (m_isStartVideo)
	{
		VideoRecorder* recorder = GetVideoRecorder();
		if (NULL != recorder)
		{
			recorder->SetCallback(this);

			// 启动录制视频
			result = recorder->StartRecord(handle, path
						, m_videoMp4Dir, m_videoCloseShell
						, m_picH264Dir, m_picDir, m_picShell, m_picInterval);
			if (result)
			{
				// 绑定handle
				handle->private_info = recorder;
				// 启动成功，插入视频处理队列
				recorder->SetVideoHandling(true);
				switch_queue_push(m_queueVideo, (void*)recorder);

				// 插入监控截图处理队列
				if (m_isStartPicture) {
					recorder->SetPicHandling(true);
					switch_queue_push(m_queuePicture, (void*)recorder);
				}
			}
			else
			{
				// 启动失败，回收recorder
				RecycleVideoRecorder(recorder);
			}
		}
	}

	if (!result) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
						, "mod_file_recorder: VideoRecordManager::StartRecord() handle:%p, path:%s, result:%d\n"
						, handle, path, result);
	}

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoRecordManager::StartRecord() end handle:%p, path:%s, result:%d\n"
//					, handle, path, result);

	return result;
}

// 停止录制
void VideoRecordManager::StopRecord(switch_file_handle_t *handle)
{
//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoRecordManager::StopRecord() start handle:%p\n"
//					, handle);

	if (NULL != handle && NULL != handle->private_info)
	{
		// 获取recorder
		VideoRecorder* recorder = (VideoRecorder*)handle->private_info;

//		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//						, "mod_file_recorder: VideoRecordManager::StopRecord() stop record, recorder:%p\n"
//						, handle);

		// 停止录制
		recorder->StopRecord();
		// 取消绑定
		handle->private_info = NULL;
	}

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoRecordManager::StopRecord() end handle:%p\n"
//					, handle);
}

// 录制视频frame
bool VideoRecordManager::RecordVideoFrame(switch_file_handle_t *handle, switch_frame_t *frame)
{
	bool result = false;

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoRecordManager::RecordVideoFrame() start handle:%p\n"
//					, handle);

	if (NULL != handle && NULL != handle->private_info)
	{
		// 获取recorder
		VideoRecorder* recorder = (VideoRecorder*)handle->private_info;

		// 录制视频
		result = recorder->RecordVideoFrame(frame);
	}
	else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
						, "mod_file_recorder: VideoRecordManager::RecordVideoFrame() fail, handle:%p, private_info:%p\n"
						, handle, handle->private_info);
	}

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoRecordManager::RecordVideoFrame() end handle:%p, result:%d\n"
//					, handle, result);

	return result;
}

// 视频录制线程
void* SWITCH_THREAD_FUNC VideoRecordManager::RecordVideoFrame2FileThread(switch_thread_t* thread, void* obj)
{
	VideoRecordManager* manager = (VideoRecordManager*)obj;
	manager->RecordVideoFrame2FileProc();
	return NULL;
}

// 视频录制线程处理函数
void VideoRecordManager::RecordVideoFrame2FileProc()
{
	void* pop = NULL;
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	VideoRecorder* recorder = NULL;
	switch_interval_time_t timeout = 500 * 1000;
	bool procResult = false;

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoRecordManager::RecordVideoFrame2FileProc() start, m_isStartVideo:%d\n"
//					, m_isStartVideo);

	while (m_isStartVideo)
	{
		status = switch_queue_pop_timeout(m_queueVideo, &pop, timeout);
		if (SWITCH_STATUS_SUCCESS == status)
		{
			recorder = (VideoRecorder*)pop;

//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//							, "mod_file_recorder: VideoRecordManager::RecordVideoFrame2FileProc() start recorder:%p\n"
//							, recorder);

			if (NULL != recorder) {
				procResult = recorder->RecordVideoFrame2FileProc();

//				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//								, "mod_file_recorder: VideoRecordManager::RecordVideoFrame2FileProc() handle finish, recorder:%p, isRecord:%d\n"
//								, recorder, recorder->IsRecord());

				if (recorder->IsRecord()) {
					// Modify by Max 2017/01/24
					// 放回处理队列之前需要释放CPU给其他录制线程处理, 否则会吃光CPU)
//					// 若没有处理，则sleep
//					if (!procResult) {
//						switch_sleep(50 * 1000);
//					}
					switch_sleep(50 * 1000);

					switch_queue_push(m_queueVideo, (void*)recorder);
				}
				else {
					// 已经停止录制，设置不再处理
					recorder->SetVideoHandling(false);
//					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//									, "mod_file_recorder: VideoRecordManager::RecordVideoFrame2FileProc() CanReset, recorder:%p, result:%d\n"
//									, recorder, recorder->CanReset());

//					if (recorder->CanReset()) {
//						// 可以重置，插入回收队列
//						switch_queue_push(m_queueToRecycle, (void*)recorder);
//					}
//					else {
//						// Modify by Max 2017/01/24
//						// 放回处理队列之前需要释放CPU给截图线程处理(当停止录制会标记为处理完成), 否则会吃光CPU
//						switch_sleep(50 * 1000);
//
//						// 未可重置，插入当前队列
//						switch_queue_push(m_queueVideo, (void*)recorder);
//					}

				}
			}
			else {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
								, "mod_file_recorder: VideoRecordManager::RecordVideoFrame2FileProc() recorder:null\n");
			}

//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//							, "mod_file_recorder: VideoRecordManager::RecordVideoFrame2FileProc() end recorder:%p\n"
//							, recorder);
		}
	}

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoRecordManager::RecordVideoFrame2FileProc() end\n");
}

// 监控截图线程
void* SWITCH_THREAD_FUNC VideoRecordManager::RecordPicture2FileThread(switch_thread_t* thread, void* obj)
{
	VideoRecordManager* manager = (VideoRecordManager*)obj;
	manager->RecordPicture2FileProc();
	return NULL;
}

// 监控截图线程处理函数
void VideoRecordManager::RecordPicture2FileProc()
{
	void* pop = NULL;
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	VideoRecorder* recorder = NULL;
	switch_interval_time_t timeout = 500 * 1000;
	bool procResult = false;

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoRecordManager::RecordPicture2FileProc() start\n");

	while (m_isStartPicture)
	{
		status = switch_queue_pop_timeout(m_queuePicture, &pop, timeout);
		if (SWITCH_STATUS_SUCCESS == status)
		{
			recorder = (VideoRecorder*)pop;

//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//							, "mod_file_recorder: VideoRecordManager::RecordPicture2FileProc() start recorder:%p\n"
//							, recorder);

			if (NULL != recorder) {
				procResult = recorder->RecordPicture2FileProc();

				if (recorder->IsRecord()) {
					// 若没有处理，则sleep
//					if (!procResult) {
						switch_sleep(50 * 1000);
//					}

					// 重新push入队列
					switch_queue_push(m_queuePicture, (void*)recorder);
				}
				else {
					recorder->SetPicHandling(false);
				}
			}
			else {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
								, "mod_file_recorder: VideoRecordManager::RecordPicture2FileProc() recorder:null\n");
			}

//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//							, "mod_file_recorder: VideoRecordManager::RecordPicture2FileProc() end recorder:%p, procResult:%d\n"
//							, recorder, procResult);
		}
	}

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoRecordManager::RecordPicture2FileProc() end\n");
}

// VideoRecorder回收线程
void* SWITCH_THREAD_FUNC VideoRecordManager::RecycleVideoRecorderThread(switch_thread_t* thread, void* obj)
{
	VideoRecordManager* manager = (VideoRecordManager*)obj;
	manager->RecycleVideoRecorderProc();
	return NULL;
}

// VideoRecorder回收线程处理函数
void VideoRecordManager::RecycleVideoRecorderProc()
{
	void* pop = NULL;
	VideoRecorder* recorder = NULL;
	switch_interval_time_t timeout = 500 * 1000;
	bool procResult = false;

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoRecordManager::RecycleVideoRecorderProc() start\n");

	while (m_isStartRecycle)
	{
		// 直到把队列处理完才能退出
		while (SWITCH_STATUS_SUCCESS == switch_queue_pop_timeout(m_queueToRecycle, &pop, timeout))
		{
			recorder = (VideoRecorder*)pop;

//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//							, "mod_file_recorder: VideoRecordManager::RecycleVideoRecorderProc() start reset, recorder:%p\n"
//							, recorder);

			if (NULL != recorder) {
				recorder->Reset();
				RecycleVideoRecorder(recorder);
			}
			else {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
								, "mod_file_recorder: VideoRecordManager::RecycleVideoRecorderProc() recorder:null\n");
			}

//			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//							, "mod_file_recorder: VideoRecordManager::RecycleVideoRecorderProc() end reset, recorder:%p\n"
//							, recorder);
		}
	}

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoRecordManager::RecycleVideoRecorderProc() end\n");
}

// 启动VideoRecorder回收线程
void VideoRecordManager::StartRecycle()
{
//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoRecordManager::StartRecycle() start\n");

	if (!m_isStartRecycle
		&& m_isInit && m_isInitConfigure)
	{
		// 初始化启动参数
		m_isStartRecycle = true;

		// 初始化线程参数
		switch_threadattr_t *thd_attr = NULL;
		switch_threadattr_create(&thd_attr, m_pool);
		switch_threadattr_detach_set(thd_attr, 1);
		switch_threadattr_stacksize_set(thd_attr, SWITCH_THREAD_STACKSIZE);
		switch_threadattr_priority_set(thd_attr, SWITCH_PRI_LOW);

		// 启动视频处理线程
		m_recycleThread = (switch_thread_t**)switch_core_alloc(m_pool, m_videoCloseThreadCount * sizeof(switch_thread_t*));
		int i = 0;
		for (i = 0; i < m_videoCloseThreadCount; i++)
		{
			switch_thread_create(&m_recycleThread[i], thd_attr, RecycleVideoRecorderThread, this, m_pool);
		}
	}

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoRecordManager::StartRecycle() end\n");
}

// 停止VideoRecorder回收线程
void VideoRecordManager::StopRecycle()
{
//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoRecordManager::StopRecycle() start\n");

	if (m_isStartRecycle)
	{
		// 设置停止
		m_isStartRecycle = false;

		// 等待VideoRecorder回收处理线程结束
		if (NULL != m_recycleThread) {
			int i;
			for (i = 0; i < m_videoCloseThreadCount; i++) {
				if (NULL != m_recycleThread[i]) {
					switch_status_t st;
					switch_thread_join(&st, m_recycleThread[i]);
				}
			}
		}
	}

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: VideoRecordManager::StopRecycle() end\n");
}

// 获取VideoRecorder
VideoRecorder* VideoRecordManager::GetVideoRecorder()
{
	VideoRecorder* recorder = NULL;
	if (SWITCH_STATUS_SUCCESS != switch_queue_trypop(m_queueFree, (void**)&recorder)) {
		recorder = new VideoRecorder;
		switch_mutex_lock(mpVideoRecorderCountMutex);
		m_videoRecorderCount++;
		switch_mutex_unlock(mpVideoRecorderCountMutex);

		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO
						, "mod_file_recorder: VideoRecordManager::GetVideoRecorder() new VideoRecorder, "
						"recorder:%p, "
						"m_videoRecorderCount:%d, "
						"queueFreeSize:%d\n"
						, recorder
						, m_videoRecorderCount
						, switch_queue_size(m_queueFree));

		// 已经不够用，拖慢
		switch_sleep(100 * 1000);
	}
	else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO
						, "mod_file_recorder: VideoRecordManager::GetVideoRecorder() success, "
						"recorder:%p, "
						"m_videoRecorderCount:%d, "
						"queueFreeSize:%d\n"
						, recorder
						, m_videoRecorderCount
						, switch_queue_size(m_queueFree));
	}
	return recorder;
}

// 回收VideoRecorder
void VideoRecordManager::RecycleVideoRecorder(VideoRecorder* recorder)
{
//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO
//					, "mod_file_recorder: VideoRecordManager::RecycleVideoRecorder() recycle VideoRecorder, "
//					"recorder:%p, "
//					"queueFreeSize:%d\n"
//					, recorder
//					, switch_queue_size(m_queueFree));

	if (NULL != recorder) {
		if( (m_videoRecorderCountMax > 0) && (switch_queue_size(m_queueFree) >= m_videoRecorderCountMax) ) {
			// 录制缓存实例限制大于0, 并且缓存已经超过限制数量, 释放
			switch_mutex_lock(mpVideoRecorderCountMutex);
			m_videoRecorderCount--;
			switch_mutex_unlock(mpVideoRecorderCountMutex);

			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO
							, "mod_file_recorder: VideoRecordManager::RecycleVideoRecorder() free VideoRecorder, "
							"recorder:%p, "
							"m_videoRecorderCount:%d, "
							"queueFreeSize:%d\n"
							, recorder
							, m_videoRecorderCount
							, switch_queue_size(m_queueFree));

			delete recorder;

		} else {
			// 放回空闲队列
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO
							, "mod_file_recorder: VideoRecordManager::RecycleVideoRecorder() recycle VideoRecorder, "
							"recorder:%p, "
							"m_videoRecorderCount:%d, "
							"queueFreeSize:%d\n"
							, recorder
							, m_videoRecorderCount
							, switch_queue_size(m_queueFree));

			switch_queue_push(m_queueFree, (void*)recorder);
		}
	}
}

void VideoRecordManager::OnStop(VideoRecorder* recorder) {
//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "VideoRecordManager::OnStop() recorder:%p\n"
//					, recorder
//					);
	// 可以重置，插入回收队列
	switch_queue_push(m_queueToRecycle, (void*)recorder);
}
