# Copyright (C) 2015 The CamShareMiddleware
# websocket client Makefile
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
CXXFLAGS +=	-I. -I../ -I../libev

LIBS =		-L. \
			-Wl,-Bdynamic -ldl -lz -lpthread 

COMMON  	=	../common/LogFile.o ../common/md5.o ../common/KThread.o ../common/ConfFile.o ../common/Arithmetic.o ../common/aes.o\
				../common/IAutoLock.o ../common/CommonFunc.o
				
OBJS	+= $(COMMON)		
OBJS	+= ISocketHandler.o IThreadHandler.o WSClient.o ws_client.o ../LogManager.o ../MessageList.o
		
TARGET =	ws-client

.PHONY: clean $(TARGET)

$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)
	@echo '################################################################'
	@echo ''
	@echo '# Bulid ws-client completed!'
	@echo ''
	@echo '################################################################'

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)