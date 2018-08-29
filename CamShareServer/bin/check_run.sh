# !/bin/sh
#
# Copyright (C) 2015 The QpidNetwork
# Camshare run shell
#
# Created on: 2016/05/23
# Author: Max.Chiu
# Email: Kingsleyyau@gmail.com
#

# define print log file path
log_file_path=$1
# print_log function
function print_log()
{
  print_body=$1
  if [ -n "$log_file_path" ]; then
    log_time=$(date "+[%Y-%m-%d %H:%M:%S]")
    echo "$log_time check_run: $print_body" >> $log_file_path
  fi
}

# define is re-run flag
is_rerun=0

# Check Camshare middle ware
APP="camshare-middleware"
PID=`ps -ef | grep ${APP} | grep -v grep | awk -F" " '{ print $2 }'`
if [ -z "$PID" ];then
  # 重启
  is_rerun=1

  # print log
  print_log "$APP is not exist"
fi

APP="camshare-executor"
PID=`ps -ef | grep ${APP} | grep -v grep | awk -F" " '{ print $2 }'`
if [ -z "$PID" ];then
  # 重启
  is_rerun=1

  # print log
  print_log "$APP is not exist"
fi

# Check freeswitch
APP="freeswitch"
PID=`ps -ef | grep ${APP} | grep -v grep | awk -F" " '{ print $2 }'`
if [ -z "$PID" ];then
  # backup log file
  TIME=$(date +%Y-%m-%d-%H-%M-%S)
  mv /usr/local/freeswitch/log/freeswitch.log /usr/local/freeswitch/log/freeswitch.log.$TIME.1

  # 重启
  is_rerun=1

  # print log
  print_log "$APP is not exist"
fi

# 重启
if [ $is_rerun = 1 ]; then
  # print log
  print_log "restart now"

  /usr/local/CamShareServer/stop.sh
  /usr/local/CamShareServer/run.sh
fi

exit $is_rerun
