# Copyright (C) 2015 The CamShareMiddleware
# rtmp client Makefile
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

CXXFLAGS +=	-Wall -fmessage-length=0 -Wunused-variable -Wno-write-strings -Wno-narrowing -Wno-unused-variable
CXXFLAGS +=	-I. -I../

# 注意连接依赖顺序问题, 被依赖的放后面
LIBS =		-L. \
			-Wl,-Bstatic -L../common -lcommon \
			-Wl,-Bstatic -L../third_party/amf -lamf \
			-Wl,-Bdynamic -ldl -lz -lpthread -lidn
				
OBJS	+=	amf.o ISocketHandler.o IThreadHandler.o RtmpClient.o rtmp_client.o
		
TARGET =	rtmp-client

DEPDIRS	:= ../third_party/amf \
			../common 
			
CLEAN_DEPS := $(addprefix _clean_, $(DEPDIRS))

.PHONY: deps clean $(TARGET)

$(TARGET):	deps $(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)
	@echo '################################################################'
	@echo ''
	@echo '# Bulid rtmp-client completed!'
	@echo ''
	@echo '################################################################'

deps:	$(DEPDIRS)
	@echo '################################################################'
	@echo ''
	@echo '# Bulid deps rtmp-client completed!'
	@echo ''
	@echo '################################################################'
	
all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)