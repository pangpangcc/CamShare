CAMSHARE_PATH := $(call my-dir)
include $(call all-subdir-makefiles, $(CAMSHARE_PATH))

LOCAL_PATH := $(CAMSHARE_PATH)

include $(CLEAR_VARS)

LOCAL_MODULE    := camshareclient

LOCAL_C_INCLUDES := $(LIBRARY_PATH)
LOCAL_C_INCLUDES += $(CAMSHARE_PATH)/ffmpeg/include

LOCAL_CPPFLAGS  := -std=c++11

LOCAL_CFLAGS = -fpermissive
LOCAL_CFLAGS += -D__STDC_CONSTANT_MACROS

LOCAL_LDLIBS := -llog
LOCAL_LDFLAGS := -L$(CAMSHARE_PATH)/ffmpeg/lib/$(TARGET_ARCH) \
					-lavcodec \
					-lavutil \
					-lswscale \
					-lavdevice \
					-lavfilter \
					-lavformat \
					-lpostproc \
					-lswresample \
					-lx264
					
LOCAL_STATIC_LIBRARIES += rtmp

REAL_PATH := $(realpath $(CAMSHARE_PATH))
LOCAL_SRC_FILES := $(call all-cpp-files-under, $(REAL_PATH))

include $(BUILD_SHARED_LIBRARY)
