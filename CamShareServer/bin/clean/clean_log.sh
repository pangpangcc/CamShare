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
# clean CamShareServer debug log
cs_debug_path="$cs_log_folder/debug"
cs_debug_file="Log*.txt"
find $cs_debug_path -mtime +$cs_debug_time -name "$cs_debug_file" -exec rm -f {} \;

