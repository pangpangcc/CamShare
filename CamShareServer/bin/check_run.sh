# !/bin/sh
#
# Copyright (C) 2015 The QpidNetwork
# Camshare run shell
#
# Created on: 2016/05/23
# Author: Max.Chiu
# Email: Kingsleyyau@gmail.com
#

# Check Camshare middle ware
APP="camshare-middleware"
PID=`ps -ef | grep ${APP} | grep -v grep | awk -F" " '{ print $2 }'`
if [ -z "$PID" ];then
  # 运行camshare-middlewarecd
	`cd /usr/local/CamShareServer/ && nohup ./camshare-middleware -f ./camshare-middleware.config 2>&1>/dev/null &`
  exit
fi

# Check freeswitch
APP="freeswitch"
PID=`ps -ef | grep ${APP} | grep -v grep | awk -F" " '{ print $2 }'`
if [ -z "$PID" ];then
  # 运行freeswitch
  sleep 5;
	`cd /usr/local/freeswitch/bin && ./freeswitch -nc`
  exit
fi