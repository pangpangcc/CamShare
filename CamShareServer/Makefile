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
CXXFLAGS = -O2 -g #-D_CHECK_MEMORY_LEAK
endif

CXXFLAGS +=	-Wall -fmessage-length=0 -Wno-unused-variable -Wno-unused-but-set-variable
CXXFLAGS +=	-I. -Iinclude -Ilibev -Iesl/include -Isqlite -Icurl/include

LIBS =		-L. \
			-Wl,-Bstatic -Llibev/.libs -lev \
			-Wl,-Bstatic -Lesl/libs -lesl \
			-Wl,-Bstatic -Lsqlite/.libs -lsqlite3 \
			-Wl,-Bstatic -Lcurl/lib/.libs -lcurl \
			-Wl,-Bdynamic -ldl -lz -lpthread -lidn

COMMON  	=	common/LogFile.o common/md5.o common/KThread.o common/ConfFile.o common/Arithmetic.o common/aes.o\
				common/IAutoLock.o common/CommonFunc.o
				
JSON    	=	json/json_reader.o json/json_value.o json/json_writer.o

AMF	    	=	amf/AMF3.o amf/AmfParser.o

XML			=   xml/tinystr.o xml/tinyxml.o xml/tinyxmlerror.o xml/tinyxmlparser.o

CHECKER		=	simulatorchecker/SimulatorProtocolTool.o

HTTPCLIENT	=	httpclient/HttpClient.o httpclient/HttpEntiy.o

REQUEST 	=	request/BaseRequest.o request/EnterConferenceRequest.o request/SendMsgExitConferenceRequest.o \
				request/SendMsgEnterConferenceRequest.o

RESPOND		=   respond/BaseRespond.o respond/DialplanRespond.o respond/RecordFinishRespond.o

LIVECHAT	=	livechat/Counter.o livechat/ILiveChatClient.o livechat/ISocketHandler.o \
				livechat/IThreadHandler.o livechat/ITransportDataHandler.o livechat/ITransportPacketHandler.o \
				livechat/LiveChatClient.o livechat/TaskManager.o livechat/TaskMapManager.o \
				livechat/TransportDataHandler.o livechat/TransportPacketHandler.o \
				livechat/task/ITask.o livechat/task/BaseTask.o \
				livechat/task/CheckVerTask.o livechat/task/RecvEnterConferenceTask.o \
				livechat/task/RecvKickUserFromConferenceTask.o \
				livechat/task/SendEnterConferenceTask.o livechat/task/SendMsgTask.o \
				livechat/task/HearbeatTask.o
				
OBJS 		=	server.o CamShareMiddleware.o TcpServer.o MessageList.o LogManager.o Client.o\
				DataHttpParser.o DataParser.o Session.o FreeswitchClient.o SessionManager.o \
				SqliteManager.o DBHandler.o
		
OBJS +=		$(COMMON)
OBJS +=		$(JSON)
OBJS +=		$(AMF)
OBJS +=		$(XML)
OBJS +=		$(CHECKER)
OBJS +=		$(HTTPCLIENT)
OBJS +=		$(REQUEST)
OBJS +=     $(RESPOND)
OBJS +=		$(LIVECHAT)
TARGET =	camshare-middleware

DEPDIRS	:= libev sqlite curl#rtmp
CLEAN_DEPS := $(addprefix _clean_, $(DEPDIRS))

.PHONY: all deps test clean cleanall install $(DEPDIRS) $(TARGET)

$(TARGET):	deps $(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)
	@echo '################################################################'
	@echo ''
	@echo '# Bulid camshare-middleware completed!'
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

test:	deps $(LIVECHAT_TEST) $(RTMP_TEST)

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
