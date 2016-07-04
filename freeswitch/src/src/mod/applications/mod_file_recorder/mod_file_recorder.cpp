/*
 * mod_file_recorder.cpp
 *
 *  Created on: 2016-04-26
 *      Author: Samson.Fan
 * Description: 录制视频文件及监控截图的freeswitch自定义模块
 */

#include <switch.h>
#include "VideoRecorder.h"

//static const char modname[] = "mod_file_record";
static char *supported_formats[SWITCH_MAX_CODECS] = { 0 };

struct video_record {
	char mp4dir[MAX_PATH_LENGTH];
	char closeshell[MAX_PATH_LENGTH];
};
typedef struct video_record video_record_t;

struct pic_record {
	int picinterval;
	char pich264dir[MAX_PATH_LENGTH];
	char picdir[MAX_PATH_LENGTH];
	char picshell[MAX_PATH_LENGTH];
};
typedef struct pic_record pic_record_t;

struct record {
	switch_file_interface_t* file_interface;
	switch_memory_pool_t* pool;
	switch_queue_t* free_recorder_queue;
	video_record_t video;
	pic_record_t pic;
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
static VideoRecorder* GetVideoRecorder();
static void RecycleVideoRecorder(VideoRecorder* recorder);

/**
 * 加载录制文件模块
 */
SWITCH_MODULE_LOAD_FUNCTION(mod_file_recorder_load)
{
	switch_api_interface_t *api_interface;
//	switch_application_interface_t *app_interface;
	int i = 0;

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "mod_file_recorder: load()\n");

	memset(&record_globals, 0, sizeof(record_globals));

//	supported_formats[i++] = "av";
//	supported_formats[i++] = "rtmp";
	supported_formats[i++] = (char*)"h264";
//	supported_formats[i++] = "mov";

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

	// 变量赋值
	record_globals.pool = pool;

	// 生成VideoRecorder可用队列
	switch_queue_create(&record_globals.free_recorder_queue, SWITCH_CORE_QUEUE_LEN, record_globals.pool);

	// 添加到api
	SWITCH_ADD_API(api_interface, "file_recorder", "custom file recorder", file_recorder_api_function, "");

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "mod_file_recorder: load() finish, pool:%p\n"
			, pool);

	return SWITCH_STATUS_SUCCESS;
}

SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_file_recorder_shutdown)
{
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "mod_file_recorder: shutdown()\n");
	return SWITCH_STATUS_SUCCESS;
}

bool LoadConfig()
{
	bool result = false;

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
					strcpy(record_globals.video.mp4dir, val);
				}
				else if (strcmp(var, "close-shell") == 0) {
					strcpy(record_globals.video.closeshell, val);
				}
				else if (strcmp(var, "pic-interval") == 0) {
					record_globals.pic.picinterval = atoi(val);
				}
				else if (strcmp(var, "pic-h264-dir") == 0) {
					strcpy(record_globals.pic.pich264dir, val);
				}
				else if (strcmp(var, "pic-dir") == 0) {
					strcpy(record_globals.pic.picdir, val);
				}
				else if (strcmp(var, "pic-shell") == 0) {
					strcpy(record_globals.pic.picshell, val);
				}
			}
		}
		switch_xml_free(xml);
	}

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG
				, "mod_file_recorder: LoadConfig() mp4dir:%s, closeshell:%s"
				  ", picinterval:%d, pich264dir:%s, picdir:%s, picshell:%s\n"
				, record_globals.video.mp4dir, record_globals.video.closeshell
				, record_globals.pic.picinterval, record_globals.pic.pich264dir, record_globals.pic.picdir, record_globals.pic.picshell);

	result = (strlen(record_globals.video.mp4dir) > 0
			&& strlen(record_globals.video.closeshell) > 0);

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

	VideoRecorder* recorder = GetVideoRecorder();
	if( NULL != recorder ) {
		handle->private_info = (void *)recorder;
		if( LoadConfig()
			&& recorder->StartRecord(handle, path, record_globals.video.mp4dir, record_globals.video.closeshell
					, record_globals.pic.pich264dir, record_globals.pic.picdir
					, record_globals.pic.picshell, record_globals.pic.picinterval) )
		{
			/**
			 * 增加支持视频
			 */
			switch_set_flag(handle, SWITCH_FILE_FLAG_VIDEO);

			// 创建文件成功
			status = SWITCH_STATUS_SUCCESS;
		}
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
					, "mod_file_recorder: close() recorder:%p\n"
					, handle->private_info);

	VideoRecorder* recorder = (VideoRecorder *)handle->private_info;
	if( recorder ) {
		recorder->StopRecord();
		RecycleVideoRecorder(recorder);
	}
	handle->private_info = NULL;

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

	VideoRecorder* recorder = (VideoRecorder *)handle->private_info;
	if( recorder ) {
		if( recorder->RecordVideoFrame(frame) ) {
			status = SWITCH_STATUS_SUCCESS;
		}
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

static VideoRecorder* GetVideoRecorder()
{
	VideoRecorder* recorder = NULL;
	if (SWITCH_STATUS_SUCCESS != switch_queue_trypop(record_globals.free_recorder_queue, (void**)&recorder)) {
		recorder = new VideoRecorder;
	}
	return recorder;
}

static void RecycleVideoRecorder(VideoRecorder* recorder)
{
	if (NULL != recorder) {
		switch_queue_trypush(record_globals.free_recorder_queue, (void*)recorder);
	}
}
