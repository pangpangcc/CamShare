/*
 * IVideoRecorder.h
 *
 *  Created on: 2018-08-29
 *      Author: Samson Fan
 * Description: 录制视频文件及监控截图接口类
 */

#ifndef SRC_MOD_APPLICATIONS_MOD_FILE_RECORDER_IVIDEORECORDER_H_
#define SRC_MOD_APPLICATIONS_MOD_FILE_RECORDER_IVIDEORECORDER_H_

#include <switch.h>

#define FILE_RECORDER_EVENT_MAINT "file_recorder::maintenance"

#define MAX_PATH_LENGTH	512

class IVideoRecorder;
class IVideoRecorderCallback {
public:
	virtual ~IVideoRecorderCallback() {};
	virtual void OnStop(IVideoRecorder* recorder) = 0;
};

class IVideoRecorder
{
public:
	IVideoRecorder() {};
	virtual ~IVideoRecorder() {};

// 对外接口函数
public:
	// 开始录制
	virtual bool StartRecord(switch_file_handle_t *handle
			, const char *videoRecPath, const char* videoDstDir, const char* finishShell
			, const char *picRecDir, const char* picDstDir, const char* picShell, int picInterval) = 0;
	// 停止录制
	virtual void StopRecord() = 0;
	// 是否正在视频录制
	virtual bool IsRecord() = 0;
	// 设置视频是否正在处理
	virtual void SetVideoHandling(bool isHandling) = 0;
	// 设置监控截图是否正在处理
	virtual void SetPicHandling(bool isHandling) = 0;
	// 录制视频frame
	virtual bool RecordVideoFrame(switch_frame_t *frame) = 0;
	// 重置(包括重置参数及执行close_shell)
	virtual void Reset() = 0;
	// 设置回调
	virtual void SetCallback(IVideoRecorderCallback* callback) = 0;

// 外部线程调用函数
public:
	// 视频录制处理函数
	virtual bool RecordVideoFrame2FileProc() = 0;
	// 监控截图处理函数
	virtual bool RecordPicture2FileProc() = 0;
};

#endif /* SRC_MOD_APPLICATIONS_MOD_FILE_RECORDER_IVIDEORECORDER_H_ */
