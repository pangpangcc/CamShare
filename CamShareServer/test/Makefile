# Copyright (C) 2015 The CamShareMiddleware
# system_test Makefile
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

CXXFLAGS +=	-Wall -fmessage-length=0 -Wunused-variable
CXXFLAGS +=	-I.

LIBS =		-L. \
			-Wl,-Bdynamic -ldl -lz -lpthread
	
COMMON  	=	common/LogFile.o common/KThread.o common/IAutoLock.o common/CommonFunc.o
		
OBJS	+= $(COMMON)	
OBJS 	+=	system_test.o

TARGET =	system_test

.PHONY: all clean $(TARGET)

$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)
	@echo '################################################################'
	@echo ''
	@echo '# Bulid system_test completed!'
	@echo ''
	@echo '################################################################'
	
all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)