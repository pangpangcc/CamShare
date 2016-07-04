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

CXXFLAGS +=	-Wall -fmessage-length=0 -Wunused-variable
CXXFLAGS +=	-I. -I../ -I../libev

LIBS =		-L. \
			-Wl,-Bdynamic -ldl -lz -lpthread 

COMMON  	=	../common/LogFile.o ../common/md5.o ../common/KThread.o ../common/ConfFile.o ../common/Arithmetic.o\
				../common/IAutoLock.o ../common/CommonFunc.o
				
OBJS	+= $(COMMON)		
OBJS	+=	amf.o ISocketHandler.o IThreadHandler.o RtmpClient.o rtmp_client.o ../LogManager.o ../MessageList.o
		
TARGET =	rtmp-client

.PHONY: clean $(TARGET)

$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)
	@echo '################################################################'
	@echo ''
	@echo '# Bulid rtmp-client completed!'
	@echo ''
	@echo '################################################################'

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)