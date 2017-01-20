# Copyright (C) 2015 The CamShareServer
# CamShare Executor Makefile
#
# Created on: 2017/01/13
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
CXXFLAGS +=	-I. -I../ -I../include -I../third_party 
CXXFLAGS += -D__STDC_CONSTANT_MACROS

# 注意连接依赖顺序问题, 被依赖的放后面
LIBS =		-L. \
			-Wl,-Bstatic -L../parser -lparser \
			-Wl,-Bstatic -L../respond -lrespond \
			-Wl,-Bstatic -L../server -lserver \
			-Wl,-Bstatic -L../common -lcommon \
			-Wl,-Bstatic -L../third_party/libev/.libs -lev \
			-Wl,-Bdynamic -ldl -lz -lpthread -lidn

RESPOND		=   respond/ExecCmdRespond.o

OBJS 		=	main.o CamShareExecutor.o #Transcoder.o

OBJS +=		$(RESPOND)
TARGET =	camshare-executor

DEPDIRS	:= ../third_party/libev \
			../common \
			../parser ../server \
			../respond 
			
CLEAN_DEPS := $(addprefix _clean_, $(DEPDIRS))

.PHONY: all deps clean install $(DEPDIRS) $(TARGET)

$(TARGET):	deps $(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)
	@echo '################################################################'
	@echo ''
	@echo '# Bulid camshare-executor completed!'
	@echo ''
	@echo '################################################################'

deps:	$(DEPDIRS)
	@echo '################################################################'
	@echo ''
	@echo '# Bulid deps camshare-executor completed!'
	@echo ''
	@echo '################################################################'
		
all:	$(TARGET)

clean:
	rm -f $(TARGET) $(OBJS) 
	
install: 
	copy camshare-executor.config /etc/ -rf
	copy camshare-executor /usr/local/bin
	chmod +x /usr/local/bin/camshare-executor
	@echo '################################################################'
	@echo ''
	@echo '# Install camshare-executor finished!'
	@echo ''
	@echo '################################################################'
