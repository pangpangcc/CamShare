CURRENT_PATH := $(call my-dir)
CAMSHARE_THIRD_PARTY_PATH := $(CURRENT_PATH)/../third_party/android

LOCAL_PATH := $(CURRENT_PATH)

include $(CLEAR_VARS)

LOCAL_MODULE    := camshare

LOCAL_CPPFLAGS  := -std=c++11
LOCAL_CPPFLAGS	+= -fpermissive -Wno-write-strings 

LOCAL_CFLAGS = -fpermissive
LOCAL_CFLAGS += -D__STDC_CONSTANT_MACROS

LOCAL_C_INCLUDES := $(LIBRARY_PATH)
LOCAL_C_INCLUDES += $(CAMSHARE_THIRD_PARTY_PATH)/ffmpeg/include

LOCAL_LDLIBS := -llog

LOCAL_STATIC_LIBRARIES += livechat
LOCAL_STATIC_LIBRARIES += common
LOCAL_STATIC_LIBRARIES += rtmp

REAL_PATH := $(realpath $(LOCAL_PATH))
LOCAL_SRC_FILES := $(call all-cpp-files-under, $(REAL_PATH))

include $(BUILD_STATIC_LIBRARY)
