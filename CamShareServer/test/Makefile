# Copyright (C) 2015 The CamShareMiddleware
# tester Makefile
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
CXXFLAGS +=	-I. -I../ \
			-I../libev \
			-I../../deps/build/include

CXXFLAGS += -D__STDC_CONSTANT_MACROS

LIBS =		-L. \
			-Wl,-Bstatic -L../libev/.libs -lev \
			-Wl,-Bstatic -L../../deps/build/lib/ \
			-lavcodec \
			-lavutil \
			-lswscale \
			-lavdevice \
			-lavfilter \
			-lavformat \
			-lpostproc \
			-lswresample \
			-lx264 \
			-Wl,-Bdynamic -ldl -lz -lpthread 

COMMON  	=	../common/LogFile.o ../common/md5.o ../common/KThread.o ../common/ConfFile.o ../common/Arithmetic.o ../common/aes.o\
				../common/IAutoLock.o ../common/CommonFunc.o
				
OBJS	+= $(COMMON)		
OBJS	+=	http_client.o HttpServerTest.o		
TARGET =	http-client

FORK_OBJS += $(COMMON)
FORK_OBJS += fork_tester.o
FORK_TEST =	fork-tester

TRANSCODER_OBJS += $(COMMON)
TRANSCODER_OBJS += transcoder_tester.o ../executor/Transcoder.o
TRANSCODER_TEST =	transcoder-tester

.PHONY: clean $(TARGET) $(FORK_TEST) $(TRANSCODER_TEST)

$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)
	@echo '################################################################'
	@echo ''
	@echo '# Bulid tester completed!'
	@echo ''
	@echo '################################################################'

$(FORK_TEST):	$(FORK_OBJS)
	$(CXX) -o $(FORK_TEST) $(FORK_OBJS) $(LIBS)
	@echo '################################################################'
	@echo ''
	@echo '# Bulid fork-tester completed!'
	@echo ''
	@echo '################################################################'
	
$(TRANSCODER_TEST):	$(TRANSCODER_OBJS)
	$(CXX) -o $(TRANSCODER_TEST) $(TRANSCODER_OBJS) $(LIBS)
	@echo '################################################################'
	@echo ''
	@echo '# Bulid transcoder-tester completed!'
	@echo ''
	@echo '################################################################'
		
all:	$(TARGET) $(FORK_TEST) $(TRANSCODER_TEST)

clean:
	rm -f $(OBJS) $(TARGET) \
	$(FORK_OBJS) $(FORK_TEST) \
	$(TRANSCODER_TEST) $(TRANSCODER_OBJS)