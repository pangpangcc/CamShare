/*
 * VideoRecordManager.h
 *
 *  Created on: 2016-08-16
 *      Author: Samson.Fan
 * Description: 录制视频管理器
 */

#ifndef SRC_MOD_APPLICATIONS_MOD_FILE_RECORDER_FILERECORDMANAGER_H_
#define SRC_MOD_APPLICATIONS_MOD_FILE_RECORDER_FILERECORDMANAGER_H_

#include <switch.h>
#include "VideoRecorder.h"

using namespace std;

class VideoRecordManager
{
public:
	VideoRecordManager();
	virtual ~VideoRecordManager();

// 对外接口函数
public:
	// 初始化
	bool Init(switch_memory_pool_t* pool);
	// 初始化配置
	bool InitConfigure(const char* videoMp4Dir, const char* videoCloseShell, int videoThreadCount, int videoCloseThreadCount
			, const char* picH264Dir, const char* picDir, const char* picShell, int picInterval, int picThreadCount);
	// 开始录制
	bool StartRecord(switch_file_handle_t *handle, const char *path);
	// 停止录制
	void StopRecord(switch_file_handle_t *handle);
	// 录制视频frame
	bool RecordVideoFrame(switch_file_handle_t *handle, switch_frame_t *frame);

// VideoRecorder回收线程函数
private:
	// VideoRecorder回收线程
	static void* SWITCH_THREAD_FUNC RecycleVideoRecorderThread(switch_thread_t* thread, void* obj);
	// VideoRecorder回收线程处理函数
	void RecycleVideoRecorderProc();
	// 启动VideoRecorder回收线程
	void StartRecycle();
	// 停止VideoRecorder回收线程
	void StopRecycle();

// 视频录制线程函数
private:
	// 视频录制线程
	static void* SWITCH_THREAD_FUNC RecordVideoFrame2FileThread(switch_thread_t* thread, void* obj);
	// 视频录制线程处理函数
	void RecordVideoFrame2FileProc();
	// 启动视频录制线程
	void StartVideo();
	// 停止视频录制线程
	void StopVideo();

// 监控截图线程函数
private:
	// 监控截图线程
	static void* SWITCH_THREAD_FUNC RecordPicture2FileThread(switch_thread_t* thread, void* obj);
	// 监控截图线程处理函数
	void RecordPicture2FileProc();
	// 启动监控截图线程
	void StartPicture();
	// 停止监控截图线程
	void StopPicture();

// VideoRecorder管理函数
private:
	// 获取VideoRecorder
	VideoRecorder* GetVideoRecorder();
	// 回收VideoRecorder
	void RecycleVideoRecorder(VideoRecorder* recorder);

// Member
private:
	// 初始化
	switch_memory_pool_t* m_pool;	// 模块内存池
	bool m_isInit;					// 是否已初始化
	bool m_isInitConfigure;			// 是否已初始化配置

	// 配置
	char m_videoMp4Dir[MAX_PATH_LENGTH];		// 视频录制完成mp4文件生成目录
	char m_videoCloseShell[MAX_PATH_LENGTH];	// 视频录制完成时执行的脚本路径
	int m_videoThreadCount;						// 视频录制处理线程数
	int m_videoCloseThreadCount;				// 视频录制完成时的处理线程数
	char m_picH264Dir[MAX_PATH_LENGTH];			// 监控截图h264文件生成目录
	char m_picDir[MAX_PATH_LENGTH];				// 监控截图文件生成目录
	char m_picShell[MAX_PATH_LENGTH];			// 监控截图生成脚本路径
	int m_picInterval;							// 监控截图生成时间间隔(毫秒)
	int m_picThreadCount;						// 监控截图处理线程数

	// VideoRecorder管理
	switch_queue_t* m_queueFree;			// VideoRecorder空闲队列
	switch_thread_t** m_recycleThread;		// VideoRecorder回收线程池
	switch_queue_t* m_queueToRecycle;		// VideoRecorder待回收队列
	bool m_isStartRecycle;					// 是否已启动VideoRecorder回收线程
	uint32_t m_videoRecorderCount;			// 已创建的VideoRecorder数量

	// 视频处理
	switch_thread_t** m_videoThread;		// 视频处理线程池
	switch_queue_t* m_queueVideo;			// 视频处理队列
	bool m_isStartVideo;					// 是否已启动视频录制线程

	// 监控截图处理
	switch_thread_t** m_picThread;			// 监控截图处理线程池
	switch_queue_t* m_queuePicture;			// 监控截图处理队列
	bool m_isStartPicture;					// 是否已启动监控截图线程
};

#endif /* SRC_MOD_APPLICATIONS_MOD_FILE_RECORDER_FILERECORDMANAGER_H_ */
