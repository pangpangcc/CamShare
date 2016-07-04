/*
 * VideoRecorder.h
 *
 *  Created on: 2016-04-26
 *      Author: Samson.Fan
 * Description: 录制视频文件及监控截图
 */

#ifndef SRC_MOD_APPLICATIONS_MOD_FILE_RECORDER_FILERECORDER_H_
#define SRC_MOD_APPLICATIONS_MOD_FILE_RECORDER_FILERECORDER_H_

#include <switch.h>

#define MAX_PATH_LENGTH	512

class VideoRecorder
{
public:
	VideoRecorder();
	virtual ~VideoRecorder();

public:
	// 开始录制
	bool StartRecord(switch_file_handle_t *handle
			, const char *path, const char* mp4dir, const char* closeshell
			, const char* pich264dir, const char* picdir, const char* picshell, int picinterval);
	// 停止录制
	void StopRecord();
	// 录制视频frame
	bool RecordVideoFrame(switch_frame_t *frame);
	// 是否正在运行
	bool IsRunning();

private:
	// 视频录制线程
	static void* SWITCH_THREAD_FUNC RecordVideoFrame2FileThread(switch_thread_t* thread, void* obj);
	void RecordVideoFrame2FileProc();
	// 监控截图线程
	static void* SWITCH_THREAD_FUNC RecordPicture2FileThread(switch_thread_t* thread, void* obj);
	void RecordPicture2FileProc();

private:
	// ----- 公共处理函数 -----
	// 重置参数
	void ResetParam();
	// 获取空闲buffer
	switch_buffer_t* GetFreeBuffer();
	// 回收空闲buffer
	void RecycleFreeBuffer(switch_buffer_t* buffer);
	// 释放空闲队列里所有buffer及空闲队列
	void DestroyFreeBufferQueue();

	// ----- 视频录制处理函数 -----
	// 解析h264，并组为h264带分隔符帧数据包
	switch_status_t buffer_h264_nalu(switch_frame_t *frame, switch_buffer_t* buffer, switch_bool_t& start);
	// 重建Nalu缓冲
	bool RenewNaluBuffer();
	// 执行关闭shell
	void RunCloseShell();
	// 获取源文件名称
	bool GetSrcPathFileName(char* buffer, const char* srcPath);
	// 获取用户名
	bool GetUserNameWithFileName(char* buffer, const char* srcPath);
	// 生成录制h264文件路径
	bool BuildH264FilePath(const char* srcPath);
	// 获取mp4文件路径
	bool GetMp4FilePath(char* mp4Path, const char* srcPath, const char* dir);
	// 把视频数据写入文件
	bool WriteVideoData2File(const uint8_t* buffer, size_t bufferLen, size_t& writeLen);
	// 释放视频录制队列里所有buffer及空闲队列
	void DestroyVideoQueue();

	// ----- 视频截图处理函数 -----
	// 生成截图的h264生成路径
	bool BuildPicH264FilePath(const char* srcPath, const char* dir);
	// 生成截图图片路径
	bool BuildPicFilePath(const char* srcPath, const char* dir);
	// 重建pic缓冲
	bool RenewPicBuffer(switch_buffer_t* buffer);
	// 生成截图
	bool BuildPicture(switch_buffer_t* buffer, uint8_t* dataBuffer, switch_size_t dataBufLen);
	// 生成截图的H264文件
	bool BuildPicH264File(switch_buffer_t* buffer, uint8_t* dataBuffer, switch_size_t dataBufLen);
	// 执行生成截图shell
	bool RunPictureShell();

private:
	bool mbRunning;
	switch_mutex_t*		mpMutex;

	char mcH264Path[2048];
	char mcMp4Dir[2048];
	char mcCloseShell[2048];

	switch_file_handle_t*	mpHandle;
	FILE*	mpFile;

	switch_queue_t*		mpVideoQueue;		// 完成组h264包的buffer队列
	switch_thread_t*	mpVideoThread;		// 视频录制线程
	switch_queue_t*		mpFreeBufferQueue;	// 空闲buffer队列

	switch_memory_pool_t*	mpMemoryPool;
	switch_bool_t 		mbNaluStart;
	switch_buffer_t*	mpNaluBuffer;

	char mcPicH264Path[2048];
	char mcPicPath[2048];
	char mcPicShell[2048];
	switch_thread_t*	mpPicThread;
	int					miPicInterval;
	switch_buffer_t*	mpPicBuffer;
	switch_mutex_t*		mpPicMutex;
};

#endif /* SRC_MOD_APPLICATIONS_MOD_FILE_RECORDER_FILERECORDER_H_ */
