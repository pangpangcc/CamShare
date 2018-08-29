CURRENT_PATH := $(call my-dir)

LOCAL_PATH := $(CURRENT_PATH)

include $(CLEAR_VARS)
LOCAL_MODULE    := camshareclient

LOCAL_C_INCLUDES := $(LIBRARY_PATH)
LOCAL_C_INCLUDES += $(CAMSHARE_PATH)

LOCAL_CPPFLAGS  := -std=c++11

LOCAL_CFLAGS = -fpermissive
LOCAL_CFLAGS += -D__STDC_CONSTANT_MACROS

LOCAL_LDLIBS := -llog 
LOCAL_LDFLAGS := -L$(CAMSHARE_THIRD_PARTY_PATH)/ffmpeg/lib/$(TARGET_ARCH_ABI)/lib \
					-lavcodec \
					-lavutil \
					-lswscale \
					-lavdevice \
					-lavfilter \
					-lavformat \
					-lpostproc \
					-lswresample \
					-lx264

LOCAL_STATIC_LIBRARIES += livechat					
LOCAL_STATIC_LIBRARIES += camshare
LOCAL_STATIC_LIBRARIES += common

REAL_PATH := $(realpath $(CURRENT_PATH))
LOCAL_SRC_FILES := $(call all-cpp-files-under, $(REAL_PATH))

include $(BUILD_SHARED_LIBRARY)
