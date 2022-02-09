# Copyright (C) 2015 The CamShareServer
# CamShareMiddleware Makefile
#
# Created on: 2015/10/10
# Author: Max.Chiu
# Email: Kingsleyyau@gmail.com
#

export MAKE	:=	make

release=0
ifeq ($(release), 1)
CXXFLAGS = -O2 
else
CXXFLAGS = -O0 -g #-D_CHECK_MEMORY_LEAK
endif

CXXFLAGS +=	-Wall -fmessage-length=0 -Wno-unused-variable -Wno-unused-but-set-variable
CXXFLAGS +=	-I. -Iinclude -Ithird_party -Ithird_party/esl/include -Ithird_party/json

# 注意连接依赖顺序问题, 被依赖的放后面
LIBS =		-L. \
			-Wl,-Bstatic -Llivechat -llivechat \
			-Wl,-Bstatic -Lparser -lparser \
			-Wl,-Bstatic -Lrequest -lrequest \
			-Wl,-Bstatic -Lrespond -lrespond \
			-Wl,-Bstatic -Lhttpclient -lhttpclient \
			-Wl,-Bstatic -Lsimulatorchecker -lsimulatorchecker \
			-Wl,-Bstatic -Lserver -lserver \
			-Wl,-Bstatic -Lcommon -lcommon \
			-Wl,-Bstatic -Lthird_party/amf -lamf \
			-Wl,-Bstatic -Lthird_party/json -ljson \
			-Wl,-Bstatic -Lthird_party/xml -lxml \
			-Wl,-Bstatic -Lthird_party/libev/.libs -lev \
			-Wl,-Bstatic -Lthird_party/esl/libs -lesl \
			-Wl,-Bstatic -Lthird_party/curl/lib/.libs -lcurl \
			-Wl,-Bstatic -Lthird_party/sqlite/.libs -lsqlite3 \
			-Wl,-Bdynamic -lssl -lcrypto -ldl -lz -lpthread -lidn -lrt

OBJS 		=	main.o CamShareMiddleware.o  \
				Session.o FreeswitchClient.o SessionManager.o \
				SqliteManager.o DBHandler.o
		
TARGET =	camshare-middleware

DEPDIRS	:= third_party/libev third_party/sqlite third_party/curl \
			third_party/amf third_party/json third_party/xml \
			common httpclient simulatorchecker \
			parser server \
			request respond livechat \
			executor
			
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
	@echo '# Bulid deps camshare-middleware completed!'
	@echo ''
	@echo '################################################################'
	
all:	deps $(TARGET) $(LIVECHAT_TEST)

test:	deps $(LIVECHAT_TEST) $(RTMP_TEST)

clean:
	rm -f $(TARGET) $(OBJS)
	
cleanall: clean	$(CLEAN_DEPS)
	@echo '################################################################'
	@echo ''
	@echo '# Clean all camshare-middleware finished!'
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
