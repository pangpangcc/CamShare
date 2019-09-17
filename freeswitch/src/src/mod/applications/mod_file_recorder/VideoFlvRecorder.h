/*
 * VideoFlvRecorder.h
 *
 *  Created on: 2016-04-26
 *      Author: Samson.Fan
 * Description: 录制flv视频文件及监控截图
 */

#ifndef SRC_MOD_APPLICATIONS_MOD_FILE_RECORDER_VIDEOFLVRECORDER_H_
#define SRC_MOD_APPLICATIONS_MOD_FILE_RECORDER_VIDEOFLVRECORDER_H_

#include <switch.h>
#include "IVideoRecorder.h"
#include "srs_librtmp.h"

class VideoFlvRecorder : public IVideoRecorder
{
public:
	// 视频帧buffer
	typedef struct _tag_video_frame_t {
		uint32_t		timestamp;
		bool			isIFrame;
		switch_buffer_t	*buffer;

		_tag_video_frame_t() {
			timestamp = 0;
			isIFrame = false;
			buffer = NULL;
		}

		void reset() {
			timestamp = 0;
			isIFrame = false;
			if (NULL != buffer) {
				switch_buffer_zero(buffer);
			}
		}
	} video_frame_t;

public:
	VideoFlvRecorder();
	virtual ~VideoFlvRecorder();

// 对外接口函数
public:
	// 开始录制
	virtual bool StartRecord(switch_file_handle_t *handle
			, const char *videoRecPath, const char* videoDstDir, const char* finishShell
			, const char *picRecDir, const char* picDstDir, const char* picShell, int picInterval);
	// 停止录制
	virtual void StopRecord();
	// 是否正在视频录制
	virtual bool IsRecord();
	// 设置视频是否正在处理
	virtual void SetVideoHandling(bool isHandling);
	// 设置监控截图是否正在处理
	virtual void SetPicHandling(bool isHandling);

	// 录制视频frame
	virtual bool RecordVideoFrame(switch_frame_t *frame);

	// 重置(包括重置参数及执行close_shell)
	virtual void Reset();

	// 设置回调
	virtual void SetCallback(IVideoRecorderCallback* callback);

// 外部线程调用函数
public:
	// 视频录制处理函数
	virtual bool RecordVideoFrame2FileProc();
	void PutVideoBuffer2FileProc();
	// 监控截图处理函数
	virtual bool RecordPicture2FileProc();

private:
	// ----- 公共处理函数 -----
	// 重置参数
	void ResetParam();
	// 获取空闲buffer
	video_frame_t* GetFreeBuffer();
	// 回收空闲buffer
	void RecycleFreeBuffer(video_frame_t* buffer);
	// 释放空闲队列里所有buffer及空闲队列
	void DestroyFreeBufferQueue();
	// 判断是否i帧
	bool IsIFrame(const uint8_t* data, switch_size_t inuse);

	// ----- 视频录制处理函数 -----
	// 解析h264，并组为h264带分隔符帧数据包
	switch_status_t buffer_h264_nalu(switch_frame_t *frame, switch_buffer_t* buffer, switch_bool_t& start);
	// 重建Nalu缓冲
	bool RenewNaluBuffer();
	// 执行关闭shell
	void RunCloseShell();
	// 根据源文件路径获取SiteID
	bool GetSiteIdWithFilePath(char* buffer, const char* srcPath);
	// 根据源文件路径获取起始时间
	bool GetStartTimeWithFilePath(char* buffer, const char* srcPath);
	// 获取标准时间格式
	bool GetStandardTime(char* buffer, const char* srcTime);
	// 根据源文件路径获取用户名
	bool GetUserIdWithFilePath(char* buffer, const char* srcPath);
	// 获取文件名中的第index个参数(从0开始)
	bool GetParamWithFileName(char* buffer, const char* fileName, int index);
	// 获取没有后缀的文件名
	bool GetFileNameWithoutExt(char* buffer, const char* filePath);
	// 生成录制flv文件路径
	bool BuildFlvFilePath(const char* srcPath);
	// 获取mp4文件路径
	bool GetMp4FilePath(char* mp4Path, const char* dir);
	// 把视频数据写入文件
	bool WriteVideoData2FlvFile(video_frame_t *frame);
	// 释放视频录制队列里所有buffer及空闲队列
	void DestroyVideoQueue();

	// ----- 视频截图处理函数 -----
	// 生成截图的视频文件生成路径
	bool BuildPicRecFilePath(const char* srcPath, const char* dir);
	// 生成截图图片路径
	bool BuildPicFilePath(const char* srcPath, const char* dir);
	// 重建pic缓冲
	bool RenewPicBuffer(video_frame_t* buffer);
	// pop出pic缓冲
	video_frame_t* PopPicBuffer();
	// 生成截图
	bool BuildPicture(video_frame_t* buffer, uint8_t* dataBuffer, switch_size_t dataBufLen);
	// 生成截图的视频文件
	bool BuildPicRecFile(video_frame_t* buffer, uint8_t* dataBuffer, switch_size_t dataBufLen);
	// 执行生成截图shell
	bool RunPictureShell();

	// 发送命令事件
	bool SendCommand(const char* cmd);
	bool file_record_send_command(const char* cmd);
	bool file_record_send_command_lua(const char* cmd);

private:
	char mcVideoRecPath[2048];
	char mcVideoDstDir[2048];
	char mcFinishShell[2048];

	switch_file_handle_t*	mpHandle;
	srs_flv_t			mpFlvFile;			// flv文件句柄

	// 视频录制处理
	int64_t	mLocalTimestamp;				// 本地的timestamp
	bool mIsRecord;							// 是否启动视频录制
	bool mHasStartRecord;					// 是否曾经启动视频录制
	bool mIsVideoHandling;					// 是否视频正在处理中
	switch_queue_t*		mpVideoQueue;		// 完成组h264包的buffer队列
	switch_queue_t*		mpFreeBufferQueue;	// 空闲buffer队列

	switch_memory_pool_t*	mpMemoryPool;	// 内存池
	switch_bool_t 		mbNaluStart;		//
	video_frame_t*		mpNaluBuffer;		// 视频帧buffer
	uint8_t* 			mpVideoDataBuffer;	// 视频文件缓存
	uint32_t			miVideoDataBufferSize;	// 视频文件缓存size
	uint32_t			miVideoDataBufferLen;	// 视频文件缓存使用长度
	// for test
	uint32_t			miCreateBufferCount;	// 创建buffer统计

	// 监控图片生成
	bool mIsPicHandling;					// 是否监控截图正在处理中
	switch_mutex_t*		mpPicMutex;			// 监控截图处理锁
	char mcPicRecPath[2048];
	char mcPicDstPath[2048];
	char mcPicShell[2048];
	int					miPicInterval;
	video_frame_t*		mpPicBuffer;		// 待生成图片的视频帧buffer
	long long			mlPicBuildTime;		// 生成图片时间
	uint8_t* 			mpPicDataBuffer;		// 监控图片flv数据缓存
	uint32_t			miPicDataBufferSize;	// 监控图片flv4数据缓存size

	// 处理回调
	IVideoRecorderCallback* mpCallback;
	switch_mutex_t*		mpMutex;		// 状态锁
};

#endif /* SRC_MOD_APPLICATIONS_MOD_FILE_RECORDER_VIDEOFLVRECORDER_H_ */
