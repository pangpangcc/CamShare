LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := rtmp

LOCAL_CPPFLAGS  := -std=c++11
LOCAL_CPPFLAGS	+= -fpermissive -Wno-write-strings

LOCAL_C_INCLUDES := $(LIBRARY_PATH)

LOCAL_LDLIBS := -llog

LOCAL_STATIC_LIBRARIES += common

REAL_PATH := $(realpath $(LOCAL_PATH))
LOCAL_SRC_FILES := $(call all-cpp-files-under, $(REAL_PATH))
LOCAL_SRC_FILES += $(call all-c-files-under, $(REAL_PATH))

include $(BUILD_STATIC_LIBRARY)
