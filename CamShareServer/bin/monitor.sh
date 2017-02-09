#!/bin/sh

# 切换到脚本目录
path=$(dirname $0)
cd $path
# log path
log_dir="./log"
#log_dir="/usr/local/CamShareServer/log/monitor"

# print log
function print_log()
{
  # make folder
  if [ ! -d $log_dir ]; then
    mkdir -p $log_dir
  fi

  # get log file path
  log_file_name=`date "+%Y%m%d"`
  log_path="$log_dir/Log$log_file_name.txt"

  # print log to file
  log_content=$1
  now_date=`date "+%Y-%m-%d %H:%M:%S"`
  echo -e "[$now_date] $log_content" >> $log_path
}

# print separator
function print_separator()
{
  print_log "--------------------------------------"
}

# print debug log
function print_debug_log()
{
  # make folder
  if [ ! -d $log_dir ]; then
    mkdir -p $log_dir
  fi

  # get log file path
  log_file_name=`date "+%Y%m%d"`
  log_debug_path="$log_dir/Log$log_file_name-debug.txt"

  # print log to file
  log_content=$1
  now_date=`date "+%Y-%m-%d %H:%M:%S"`
  echo -e "[$now_date] $log_content" >> $log_debug_path
}

# print debug separator
function print_debug_separator()
{
  print_debug_log "--------------------------------------"
}

# memory monitor
function print_memory_log()
{
#  fs_mem=`ps -e -o 'comm,pcpu,rsz'|grep freeswitch`
  fs_mem=`top -b -n 1|grep freeswitch`
  print_log "$fs_mem"
#  cs_mem=`ps -e -o 'comm,pcpu,rsz'|grep camshare-m`
  cs_mem=`top -b -n 1|grep camshare-m`
  print_log "$cs_mem"
  cx_mem=`top -b -n 1|grep camshare-e`
  print_log "$cx_mem"
}

# freeswitch conference monitor 
function print_fs_conference_log()
{
  fs_conference_count=`/usr/local/freeswitch/bin/fs_cli -t 20 -T 20 -x "conference list"|grep "Conference"|wc -l`
  print_log "fs conference count: $fs_conference_count"

  #fs_pich264_count=`ls -l /usr/local/freeswitch/recordings/pic_jpg/|wc -l`
  #print_log "fs pic-h264 count: $fs_pich264_count"

  fs_conference=`/usr/local/freeswitch/bin/fs_cli -t 20 -T 20 -x "conference list"|grep "Conference"`
  print_log "fs conference list:\n$fs_conference"
}

# netstat 
function print_netstat_rtmp_log()
{
  rtmp_count=`netstat -atnl | grep 1935 | grep -v 0.0.0.0 | wc -l`
  print_log "rtmp count : $rtmp_count"
}

function print_netstat_websocket_log()
{
  ws_count=`netstat -atnl | grep 8080 | grep -v 0.0.0.0 | wc -l`
  print_log "websocket count : $ws_count"
}

# all process monitor
function print_all_process_log()
{
#  all_mem=`ps -e -o 'comm,pcpu,rsz'`
  all_mem=`top -b -n 1`
  print_debug_log "$all_mem"
}

# function main
function main()
{
  while true
  do
    print_separator
    print_memory_log
    print_netstat_rtmp_log
    print_netstat_websocket_log
    print_fs_conference_log
    #print_debug_separator
    #print_all_process_log
    sleep 30
  done
}

# runing
main
