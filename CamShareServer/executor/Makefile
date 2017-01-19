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
CXXFLAGS +=	-I. -I../ -Iinclude \
			-I../libev 
			#-I../../deps/build/include

CXXFLAGS += -D__STDC_CONSTANT_MACROS

LIBS =		-L. \
			-Wl,-Bstatic -L../libev/.libs -lev \
			-Wl,-Bdynamic -ldl -lz -lpthread

COMMON  	=	../common/LogFile.o ../common/md5.o ../common/KThread.o ../common/ConfFile.o ../common/Arithmetic.o ../common/aes.o\
				../common/IAutoLock.o ../common/CommonFunc.o ../common/LogManager.o
				
JSON    	=	../json/json_reader.o ../json/json_value.o ../json/json_writer.o

SERVER		= 	../server/TcpServer.o ../server/AsyncIOServer.o

PARSER		= 	../parser/DataParser.o ../parser/HttpParser.o 

RESPOND		=   ../respond/BaseRespond.o respond/ExecCmdRespond.o

OBJS 		=	main.o CamShareExecutor.o #Transcoder.o

OBJS +=		$(COMMON)
OBJS +=		$(JSON)
OBJS +=		$(SERVER)
OBJS +=		$(PARSER)
OBJS +=		$(RESPOND)
TARGET =	camshare-executor

.PHONY: all clean cleanall install $(TARGET)

$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)
	@echo '################################################################'
	@echo ''
	@echo '# Bulid camshare-executor completed!'
	@echo ''
	@echo '################################################################'
	
all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
	
cleanall: clean
	@echo '################################################################'
	@echo ''
	@echo '# Clean all finished!'
	@echo ''
	@echo '################################################################'
	
install: 
	copy camshare-executor.config /etc/ -rf
	copy camshare-executor /usr/local/bin
	chmod +x /usr/local/bin/camshare-executor
	@echo '################################################################'
	@echo ''
	@echo '# Install camshare-executor finished!'
	@echo ''
	@echo '################################################################'
