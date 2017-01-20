# Copyright (C) 2015 The CamShareServer
# livechat module Makefile
#
# Created on: 2015/10/10
# Author: Max.Chiu
# Email: Kingsleyyau@gmail.com
#

export MAKE	:=	make

release=0
ifeq ($(release), 1)
CXXFLAGS = -O3 
else
CXXFLAGS = -O2 -g
endif

CXXFLAGS +=	-Wall -fmessage-length=0 -Wno-unused-variable -Wno-unused-but-set-variable
CXXFLAGS +=	-I. -I.. -I../include -I../third_party

LIBS	=	-L. \
			-Wl,-Bstatic -L../curl/lib/.libs -lcurl \
			
OBJS 		=	Counter.o ILiveChatClient.o ISocketHandler.o \
				IThreadHandler.o ITransportDataHandler.o ITransportPacketHandler.o \
				LiveChatClient.o TaskManager.o TaskMapManager.o \
				TransportDataHandler.o TransportPacketHandler.o \
				task/ITask.o task/BaseTask.o \
				task/CheckVerTask.o task/RecvEnterConferenceTask.o \
				task/RecvKickUserFromConferenceTask.o \
				task/SendEnterConferenceTask.o task/SendMsgTask.o \
				task/HearbeatTask.o
		
TARGET =	liblivechat.a

.PHONY: all clean $(TARGET)

$(TARGET):	$(OBJS)
	$(AR) cr $@ $(OBJS)
	@echo '################################################################'
	@echo ''
	@echo '# Bulid liblivechat.a completed!'
	@echo ''
	@echo '################################################################'	
	
all:	$(TARGET) 

clean:
	rm -f $(OBJS) $(TARGET)