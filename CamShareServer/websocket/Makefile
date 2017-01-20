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
CXXFLAGS +=	-I. -I../

# 注意连接依赖顺序问题, 被依赖的放后面
LIBS =		-L. \
			-Wl,-Bstatic -L../common -lcommon \
			-Wl,-Bdynamic -ldl -lz -lpthread -lidn
				
OBJS	+= ISocketHandler.o IThreadHandler.o WSClient.o ws_client.o
		
TARGET =	ws-client

DEPDIRS	:= 	../common 
			
CLEAN_DEPS := $(addprefix _clean_, $(DEPDIRS))

.PHONY: deps clean $(TARGET)

$(TARGET):	deps $(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)
	@echo '################################################################'
	@echo ''
	@echo '# Bulid ws-client completed!'
	@echo ''
	@echo '################################################################'
	
deps:	$(DEPDIRS)
	@echo '################################################################'
	@echo ''
	@echo '# Bulid deps ws-client completed!'
	@echo ''
	@echo '################################################################'
all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)