# !/bin/sh
#
# Copyright (C) 2015 The QpidNetwork
# Camshare run shell
#
# Created on: 2016/05/23
# Author: Max.Chiu
# Email: Kingsleyyau@gmail.com
#

PROCESS_CMD="./run.sh"

# Check Camshare middle ware
APP="camshare-middleware"
PID=`ps -ef | grep ${APP} | grep -v grep`
if [ -z "$PID" ];then
  ${PROCESS_CMD}
  exit
fi

# Check freeswitch
APP="freeswitch"
PID=`ps -ef | grep ${APP} | grep -v grep`
if [ -z "$PID" ];then
  ${PROCESS_CMD}
  exit
fi