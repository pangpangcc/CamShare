/*
 * mod_file_recorder.cpp
 *
 *  Created on: 2016-04-26
 *      Author: Samson.Fan
 * Description: 录制视频文件及监控截图的freeswitch自定义模块
 */

#include <switch.h>
//#include "VideoH264Recorder.h"
//#include "VideoFlvRecorder.h"
#include "VideoRecordManager.h"

//static const char modname[] = "mod_file_record";
static char *supported_formats[SWITCH_MAX_CODECS] = { 0 };

// 视频录制管理器
static VideoRecordManager g_videoRecordMgr;

// 初始化global参数
struct record {
	switch_file_interface_t* file_interface;
};
typedef struct record record_t;
static record_t record_globals;

SWITCH_MODULE_LOAD_FUNCTION(mod_file_recorder_load);
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_file_recorder_shutdown);
SWITCH_MODULE_DEFINITION(mod_file_recorder, mod_file_recorder_load, mod_file_recorder_shutdown, NULL);
SWITCH_STANDARD_API(file_recorder_api_function);

// 加载配置函数
bool LoadConfig();

/**
 * switch_file_interface_t接口定义
 */
static switch_status_t record_file_open(switch_file_handle_t *handle, const char *path);
static switch_status_t record_file_close(switch_file_handle_t *handle);
static switch_status_t record_file_truncate(switch_file_handle_t *handle, int64_t offset);
static switch_status_t record_file_read(switch_file_handle_t *handle, void *data, size_t *len);
static switch_status_t record_file_write(switch_file_handle_t *handle, void *data, size_t *len);
static switch_status_t record_file_read_video(switch_file_handle_t *handle, switch_frame_t *frame, switch_video_read_flag_t flags);
static switch_status_t record_file_write_video(switch_file_handle_t *handle, switch_frame_t *frame);
static switch_status_t record_file_seek(switch_file_handle_t *handle, unsigned int *cur_sample, int64_t samples, int whence);
static switch_status_t record_file_set_string(switch_file_handle_t *handle, switch_audio_col_t col, const char *string);
static switch_status_t record_file_get_string(switch_file_handle_t *handle, switch_audio_col_t col, const char **string);

/**
 * free_recorder_queue处理函数
 */
//static VideoRecorder* GetVideoRecorder();
//static void RecycleVideoRecorder(VideoRecorder* recorder);

/**
 * 加载录制文件模块
 */
SWITCH_MODULE_LOAD_FUNCTION(mod_file_recorder_load)
{
	switch_api_interface_t *api_interface;
//	switch_application_interface_t *app_interface;
	int i = 0;

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "mod_file_recorder: load()\n");

//	supported_formats[i++] = "av";
//	supported_formats[i++] = "rtmp";
//	supported_formats[i++] = (char*)"h264";
	supported_formats[i++] = (char*)"flv";
//	supported_formats[i++] = "mov";

	/* create/register custom event message type */
	if (switch_event_reserve_subclass(FILE_RECORDER_EVENT_MAINT) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't register subclass %s!\n", FILE_RECORDER_EVENT_MAINT);
		return SWITCH_STATUS_TERM;
	}

	// 创建模块
	*module_interface = switch_loadable_module_create_module_interface(pool, modname);

	// 创建模块接口
	record_globals.file_interface = (switch_file_interface_t *)switch_loadable_module_create_interface(*module_interface, SWITCH_FILE_INTERFACE);
	record_globals.file_interface->interface_name = modname;
	record_globals.file_interface->extens = supported_formats;
	record_globals.file_interface->file_open = record_file_open;
	record_globals.file_interface->file_close = record_file_close;
	record_globals.file_interface->file_truncate = record_file_truncate;
	record_globals.file_interface->file_read = record_file_read;
	record_globals.file_interface->file_write = record_file_write;
	record_globals.file_interface->file_read_video = record_file_read_video;
	record_globals.file_interface->file_write_video = record_file_write_video;
	record_globals.file_interface->file_seek = record_file_seek;
	record_globals.file_interface->file_set_string = record_file_set_string;
	record_globals.file_interface->file_get_string = record_file_get_string;

	// 添加到api
	SWITCH_ADD_API(api_interface, "file_recorder", "custom file recorder", file_recorder_api_function, "");

	// 初始化VideoRecordManager
	g_videoRecordMgr.Init(pool);

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "mod_file_recorder: load() finish, pool:%p\n"
//			, pool);

	return SWITCH_STATUS_SUCCESS;
}

SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_file_recorder_shutdown)
{
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "mod_file_recorder: shutdown()\n");

	switch_event_free_subclass(FILE_RECORDER_EVENT_MAINT);

	return SWITCH_STATUS_SUCCESS;
}

bool LoadConfig()
{
	bool result = false;

	// 视频处理参数
	char mp4Dir[MAX_PATH_LENGTH] = {0};
	char closeShell[MAX_PATH_LENGTH] = {0};
	int videoThreadCount = 0;
	int videoCloseThreadCount = 1;
	// 监控图片处理参数
	char picH264Dir[MAX_PATH_LENGTH] = {0};
	char picDir[MAX_PATH_LENGTH] = {0};
	char picShell[MAX_PATH_LENGTH] = {0};
	int picInterval = 0;
	int picThreadCount = 0;
	int videoRecorderCountMax = 0;

	const char *cf = "file_recorder.conf";
	switch_xml_t cfg, xml, settings, param;

	if (!(xml = switch_xml_open_cfg(cf, &cfg, NULL))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "mod_file_recorder: Open of %s failed\n", cf);
	} else {
		if ((settings = switch_xml_child(cfg, "settings"))) {
			for (param = switch_xml_child(settings, "param"); param; param = param->next) {
				char *var = (char *) switch_xml_attr_soft(param, "name");
				char *val = (char *) switch_xml_attr_soft(param, "value");

//				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//						, "mod_file_recorder: name:%s, value:%s\n", var, val);

				if (strcmp(var, "mp4-dir") == 0) {
					strcpy(mp4Dir, val);
				}
				else if (strcmp(var, "close-shell") == 0) {
					strcpy(closeShell, val);
				}
				else if (strcmp(var, "video-thread") == 0) {
					videoThreadCount = atoi(val);
				}
				else if (strcmp(var, "video-close-thread") == 0) {
					videoCloseThreadCount = atoi(val);
				}
				else if (strcmp(var, "pic-interval") == 0) {
					picInterval = atoi(val);
				}
				else if (strcmp(var, "pic-h264-dir") == 0) {
					strcpy(picH264Dir, val);
				}
				else if (strcmp(var, "pic-dir") == 0) {
					strcpy(picDir, val);
				}
				else if (strcmp(var, "pic-shell") == 0) {
					strcpy(picShell, val);
				}
				else if (strcmp(var, "pic-thread") == 0) {
					picThreadCount = atoi(val);
				} else if (strcmp(var, "video-recorder-count-max") == 0) {
					videoRecorderCountMax = atoi(val);
				}
			}
		}
		switch_xml_free(xml);
	}

	result = g_videoRecordMgr.InitConfigure(mp4Dir, closeShell, videoThreadCount, videoCloseThreadCount
				, picH264Dir, picDir, picShell, picInterval, picThreadCount, videoRecorderCountMax);

	if (!result) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
					, "mod_file_recorder: LoadConfig() fail, mp4dir:%s, closeshell:%s, videothread:%d, videoCloseThread:%d"
					  ", picthread:%d, picinterval:%d, pich264dir:%s, picdir:%s, picshell:%s"
					  ", result:%d\n"
					, mp4Dir, closeShell, videoThreadCount, videoCloseThreadCount
					, picThreadCount, picInterval, picH264Dir, picDir, picShell
					, result);
	}

	return result;
}

/**
 * api调用
 */
SWITCH_STANDARD_API(file_recorder_api_function)
{
	stream->write_function(stream, "custom file recorder is running");

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t record_file_open(switch_file_handle_t *handle, const char *path)
{
	switch_status_t status = SWITCH_STATUS_FALSE;

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
					, "mod_file_recorder: open() handle:%p, path:%s\n"
					, handle, path);

	// 开始录制
	if( LoadConfig()
		&& g_videoRecordMgr.StartRecord(handle, path) )
	{
		/**
		 * 增加支持视频
		 */
		switch_set_flag(handle, SWITCH_FILE_FLAG_VIDEO);

		// 创建文件成功
		status = SWITCH_STATUS_SUCCESS;
	}
	else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
						, "mod_file_recorder: open() fail, handle:%p, path:%s\n"
						, handle, path);
	}

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
					, "mod_file_recorder: open() handle:%p, status:%d\n"
					, handle, status);

	return status;
}

static switch_status_t record_file_close(switch_file_handle_t *handle)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
					, "mod_file_recorder: close() handle:%p\n"
					, handle);

	// 停止录制
	g_videoRecordMgr.StopRecord(handle);

	return status;
}

static switch_status_t record_file_truncate(switch_file_handle_t *handle, int64_t offset)
{
	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t record_file_read(switch_file_handle_t *handle, void *data, size_t *len)
{
	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t record_file_write(switch_file_handle_t *handle, void *data, size_t *len)
{
	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t record_file_read_video(switch_file_handle_t *handle, switch_frame_t *frame, switch_video_read_flag_t flags)
{
	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t record_file_write_video(switch_file_handle_t *handle, switch_frame_t *frame)
{
	switch_status_t status = SWITCH_STATUS_FALSE;

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: write_video() handle:%p, frame:%p\n"
//					, handle, frame);

	if( g_videoRecordMgr.RecordVideoFrame(handle, frame) ) {
		status = SWITCH_STATUS_SUCCESS;
	}
	else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR
						, "mod_file_recorder: write_video() fail, handle:%p\n"
						, handle);
	}

//	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
//					, "mod_file_recorder: write_video() handle:%p, status:%d\n"
//					, handle, status);

	return status;
}

static switch_status_t record_file_seek(switch_file_handle_t *handle, unsigned int *cur_sample, int64_t samples, int whence)
{
	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t record_file_set_string(switch_file_handle_t *handle, switch_audio_col_t col, const char *string)
{
	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t record_file_get_string(switch_file_handle_t *handle, switch_audio_col_t col, const char **string)
{
	return SWITCH_STATUS_SUCCESS;
}
