#!/bin/sh

# define reboot log path
reboot_log_file_path=$1
# print_log function
function print_log()
{
  print_body=$1
  if [ -n "$reboot_log_file_path" ]; then
    log_time=$(date "+[%Y-%m-%d %H:%M:%S]")
    echo "$log_time check_makecall_fail: $print_body" >> $reboot_log_file_path
  fi
}

# define check log path
log_folder="/usr/local/freeswitch/log"
log_file_name="freeswitch.log"
log_file_path="$log_folder/$log_file_name"
# check log file is exist
if [ ! -e "$log_file_path" ]; then
  exit 0
fi

# define key
start_key="内网拨号计划->结束"
end_key="会议事件监听脚本->事件:channel_id"
timeout_s=5

# parsing data
last_line=$(tail -n 1 $log_file_path)
last_start_key_line=$(grep "$start_key" $log_file_path| tail -n 1)
last_end_key_line=$(grep "$end_key" $log_file_path| tail -n 1)
#echo "last_line: $last_line"
#echo "last_start_key_line: $last_start_key_line"
#echo "last_end_key_line: $last_end_key_line"
#echo "tail -n 1 $log_file_path"

# check whether reboot
is_reboot=0
if [ -n "$last_line" ] && [ -n "$last_start_key_line" ]; then
  # get last start time
  last_start_time="0"
  last_start_date_str=$(echo "$last_start_key_line"|awk '{print $2}')
  last_start_time_str=$(echo "$last_start_key_line"|awk '{print $3}')
  last_start_time=$(date -d "$last_start_date_str $last_start_time_str" +%s)
#  echo "last_start_time: $last_start_time"
  
  # get last end time
  last_end_time="0"
  if [ -n "$last_end_key_line" ]; then
    last_end_date_str=$(echo "$last_end_key_line"|awk '{print $1}')
    last_end_time_str=$(echo "$last_end_key_line"|awk '{print $2}')
    last_end_time=$(date -d "$last_end_date_str $last_end_time_str" +%s)
  fi
#  echo "last_end_time: $last_end_time"

  # get last line time
  last_line_time="0"
  last_line_date_utc=0
  # check the first and the second field whether 'date time'
  last_line_date_str=$(echo "$last_line"|awk '{print $1}'|grep "-")
  last_line_time_str=$(echo "$last_line"|awk '{print $2}'|grep ":")
  # to get last line time
  if [ -n "$last_line_date_str" ] && [ -n "$last_line_time_str" ]; then
    # the first and the second field is 'date time'
    last_line_time=$(date -d "$last_line_date_str $last_line_time_str" +%s)
  else
    last_line_date_str=$(echo "$last_line"|awk '{print $2}'|grep "-")
    last_line_time_str=$(echo "$last_line"|awk '{print $3}'|grep ":")
    if [ -n "$last_line_date_str" ] && [ -n "$last_line_time_str" ]; then
      # the second and the third field is 'date time'
      last_line_time=$(date -d "$last_line_date_str $last_line_time_str" +%s)
    fi
  fi
#  echo "last_line_time: $last_line_time"

  # check reboot
#  echo "last_start_time:$last_start_time, last_end_time:$last_end_time, last_line_time:$last_line_time, timeout_s:$timeout_s, timeout:$(($last_start_time+$timeout_s))"
  if [ $(($last_start_time)) -gt $(($last_end_time)) ]; then
#    echo "$(($last_start_time)) -gt $(($last_end_time)) true"
    if [ $(($last_line_time)) -gt $(($last_start_time+$timeout_s)) ]; then
      is_reboot=1
#      echo "is_reboot: $is_reboot"
    fi
  fi
fi

# reboot
if [ $(($is_reboot)) -gt 0 ]; then
  # print log
  print_log "make call fail, reboot now"
  
  # Stop server
  /usr/local/CamShareServer/stop.sh
  # Change log file name
  reboot_time=$(date +%Y-%m-%d-%H-%M-%S)
  mv $log_folder/$log_file_name $log_folder/$log_file_name.$reboot_time.1
  # Start server
  /usr/local/CamShareServer/run.sh

  # throw exception, exit
  exit 1
fi

