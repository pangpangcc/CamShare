# Copyright (C) 2015 The CamShareMiddleware
# CamShareMiddleware Makefile
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

CXXFLAGS +=	-Wall -fmessage-length=0 -Wunused-variable
CXXFLAGS +=	-I. -Iinclude -Ilibev -Iesl/include

LIBS =		-L. \
			-Wl,-Bstatic -Llibev/.libs -lev \
			-Wl,-Bstatic -Lesl/libs -lesl \
			-Wl,-Bdynamic -ldl -lz -lpthread 

COMMON  	=	common/LogFile.o common/md5.o common/KThread.o common/ConfFile.o common/Arithmetic.o\
				common/IAutoLock.o common/CheckMomoryLeak.o common/CommonFunc.o
				
JSON    	=	json/json_reader.o json/json_value.o json/json_writer.o

AMF	    	=	amf/AMF3.o amf/AmfParser.o

XML			=   xml/tinystr.o xml/tinyxml.o xml/tinyxmlerror.o xml/tinyxmlparser.o

REQUEST 	=	request/BaseRequest.o request/EnterConferenceRequest.o

RESPOND		=   respond/BaseRespond.o respond/DialplanRespond.o

LIVECHAT	=	livechat/Counter.o livechat/ILiveChatClient.o livechat/ISocketHandler.o \
				livechat/IThreadHandler.o livechat/ITransportDataHandler.o livechat/ITransportPacketHandler.o \
				livechat/LiveChatClient.o livechat/TaskManager.o livechat/TaskMapManager.o \
				livechat/TransportDataHandler.o livechat/TransportPacketHandler.o \
				livechat/task/ITask.o livechat/task/BaseTask.o livechat/task/CheckVerTask.o livechat/task/SendEnterConferenceTask.o \
				livechat/task/RecvDisconnectUserVideo.o
				
OBJS 		=	server.o CamShareMiddleware.o TcpServer.o MessageList.o LogManager.o Client.o\
				DataHttpParser.o DataParser.o Session.o FreeswitchClient.o
		
OBJS +=		$(COMMON)
OBJS +=		$(JSON)
OBJS +=		$(AMF)
OBJS +=		$(XML)
OBJS +=		$(REQUEST)
OBJS +=     $(RESPOND)
OBJS +=		$(LIVECHAT)

TARGET =	camshare-middleware

LIVECHAT_TEST_OBJ = $(COMMON)
LIVECHAT_TEST_OBJ += livechat_tester/livechat_tester.o TcpServer.o MessageList.o LogManager.o
LIVECHAT_TEST = livechat-tester

DEPDIRS	:= libev
CLEAN_DEPS := $(addprefix _clean_, $(DEPDIRS))

.PHONY: all deps test clean cleanall install $(DEPDIRS) $(TARGET)

$(TARGET):	deps $(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)
	@echo '################################################################'
	@echo ''
	@echo '# Bulid camshare-middleware completed!'
	@echo ''
	@echo '################################################################'

$(LIVECHAT_TEST):	deps $(LIVECHAT_TEST_OBJ)
	$(CXX) -o $(LIVECHAT_TEST) $(LIVECHAT_TEST_OBJ) $(LIBS)
	@echo '################################################################'
	@echo ''
	@echo '# Bulid livechat-tester completed!'
	@echo ''
	@echo '################################################################'
	
$(DEPDIRS):
	$(MAKE) -C $@
	
$(CLEAN_DEPS):	
	$(MAKE) -C $(patsubst _clean_%, %, $@) clean
				
deps:	$(DEPDIRS)
	@echo '################################################################'
	@echo ''
	@echo '# Bulid deps completed!'
	@echo ''
	@echo '################################################################'
	
all:	deps $(TARGET) $(LIVECHAT_TEST)

test:	deps $(LIVECHAT_TEST)

clean:
	rm -f $(OBJS) $(TARGET)
	
cleanall: clean	$(CLEAN_DEPS)
	@echo '################################################################'
	@echo ''
	@echo '# Clean all finished!'
	@echo ''
	@echo '################################################################'
	
install: 
	copy camshare-middleware.config /etc/ -rf
	copy camshare-middleware /usr/local/bin
	chmod +x /usr/local/bin/camshare-middleware
	@echo '################################################################'
	@echo ''
	@echo '# Install camshare-middleware finished!'
	@echo ''
	@echo '################################################################'
