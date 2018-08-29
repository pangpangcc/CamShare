#!/bin/sh

# time define
fs_log_time="90"
cs_debug_time="90"

# clean freeswitch log
fs_log_path="/usr/local/freeswitch/log"
fs_log_file="freeswitch.log.*.*"
find $fs_log_path -mtime +$fs_log_time -name "$fs_log_file" -exec rm -f {} \;

# define CamShareServer log path
cs_log_folder="/usr/local/CamShareServer/log"
# clean camshare_xxx debug log
function clean_debug_log()
{
  app_name=$1
  cs_debug_path=$cs_log_folder/$app_name/debug
  cs_debug_file="Log*.txt"
  find $cs_debug_path -mtime +$cs_debug_time -name "$cs_debug_file" -exec rm -f {} \;
}

# clean camshare_server debug log
cs_svr_name=camshare_server
clean_debug_log $cs_svr_name
# clean camshare_executor debug log
cs_exe_name=camshare_executor
clean_debug_log $cs_exe_name

