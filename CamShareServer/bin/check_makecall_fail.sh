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
  #echo "$log_time check_makecall_fail.sh: $print_body"
}

# define check log path
log_folder="/usr/local/freeswitch/log"
log_file_name="freeswitch.log"
log_file_path="$log_folder/$log_file_name"

#log_file_path=$1
# check log file is exist
if [ ! -e "$log_file_path" ]; then
  echo "# $log_file_path is not exist"
  exit 0
fi

log_file_path_last=$(ls -alht /app/freeswitch/log/freeswitch.log.* | head -n 1 | awk '{print $9}')
log_file_path_last_modify_time=0
if [ "$log_file_path_last" != "" ];then
  log_file_path_last_modify_time=$(stat -c %Y $log_file_path_last)
fi

# 获取单行日志时间
function get_log_datetime() {
  line=$1
  
  line_date_time=0;
  # check the first and the second field whether 'date time'
  line_date_str=$(echo "$line"|awk '{print $1}'|grep "-")
  line_time_str=$(echo "$line"|awk '{print $2}'|grep ":")
  # to get last line time
  if [ -n "$line_date_str" ] && [ -n "$line_time_str" ]; then
    # the first and the second field is 'date time'
    line_date_time=$(date -d "$line_date_str $line_time_str" +%s)
  else
    line_date_str=$(echo "$line"|awk '{print $2}'|grep "-")
    line_time_str=$(echo "$line"|awk '{print $3}'|grep ":")
    if [ -n "$line_date_str" ] && [ -n "$line_time_str" ]; then
      # the second and the third field is 'date time'
      line_date_time=$(date -d "$line_date_str $line_time_str" +%s)
    fi
  fi
  echo "$line_date_time"
}

# 开始
print_log "# Check() Start #################################################################################### "

# 检查端口是否联通
#CHECK_WS_PORT=$(timeout 3 bash -c 'cat < /dev/null > /dev/tcp/127.0.0.1/8080')
#echo $CHECK_WS_PORT | grep $CHECK_WS_PORT

# 死锁检查间隔
DEADLOCK_TIMEOUT_S=5

# 获取死锁开始特征时间
deadlock_start_key="内网拨号计划->结束"
deadlock_start_key_line=$(grep "$deadlock_start_key" $log_file_path | tail -n 1)
print_log "# Check.Deadlock() deadlock_start_key_line:$deadlock_start_key_line"

# 是否需要检查死锁
is_check_deadlock=0
deadlock_start_time=0
deadloack_end_time=0
if [ -n "$deadlock_start_key_line" ]; then  
  is_check_deadlock=1
  
  # 获取死锁结束特征时间
  deadlock_channel=$(echo "$deadlock_start_key_line"|awk '{print $1}')
  deadlock_start_time=$(get_log_datetime "$deadlock_start_key_line")
  print_log "# Check.Deadlock() deadlock_channel:$deadlock_channel"
  print_log "# Check.Deadlock() deadlock_start_time: $deadlock_start_time"
  
  deadlock_end_key="会议事件监听脚本->事件:channel_id"
  deadlock_end_key_line=$(grep "$deadlock_end_key" $log_file_path | grep $deadlock_channel | tail -n 1)
  print_log "# Check.Deadlock() deadlock_end_key_line:$deadlock_end_key_line"

  deadlock_end_key_ws="WebSocketServer::WSOnDestroy"
  deadlock_end_key_line_ws=$(grep "$deadlock_end_key_ws" $log_file_path | grep $deadlock_channel | tail -n 1)
  print_log "# Check.Deadlock() deadlock_end_key_line_ws:$deadlock_end_key_line_ws"
  
  if [ "$deadloack_end_time" == 0 ] && [ -n "$deadlock_end_key_line" ]; then
    deadloack_end_time=$(get_log_datetime "$deadlock_end_key_line")
  fi
  if [ "$last_end_time" == 0 ] && [ -n "$deadlock_end_key_line_ws" ]; then
    deadloack_end_time=$(get_log_datetime "$deadlock_end_key_line_ws")
  fi
fi  

# 获取登录特征时间
login_key="用户登录脚本->结束"
login_key_line=$(grep "$login_key" $log_file_path | tail -n 1)
print_log "# Check.Login() login_key_line:$login_key_line"

# 获取最后单行日志时间
last_line=$(tail -n 1 $log_file_path)
last_line_time=$(get_log_datetime "$last_line")
print_log "# Check() last_line:$last_line"
print_log "# Check() last_line_time:$last_line_time"

# 判断是否特征死锁
is_reboot=0
if [ is_check_deadlock == 1 ]; then
  print_log "# Check.Deadlock() deadlock_start_time:$deadlock_start_time deadloack_end_time:$deadloack_end_time deadlock_delta_1:$((${deadloack_end_time}-${deadlock_start_time})) deadlock_delta_2:$((${last_line_time}-${deadlock_start_time})))"
  
  # 是否超过死锁检查间隔
  if [ $(($deadlock_start_time)) -gt $(($deadloack_end_time)) ]; then
    if [ $(($last_line_time)) -gt $(($deadlock_start_time+$DEADLOCK_TIMEOUT_S)) ]; then
      print_log "# Reboot because deadlock keys check timeout."
      is_reboot=1
    fi
  fi
fi

# 检查命令是否超时
if [ $(($is_reboot)) == 0 ]; then
  i=0;
  timeout=10000;
  for ((i=0; i<3; i++))
  do
    fs_conference_cmd=`/usr/local/freeswitch/bin/fs_cli -t $timeout -T $timeout -x "conference list" 2>&1` 
    fs_conference_result=`echo $fs_conference_cmd | grep "Request timed out.\|Error Connecting"` 
    print_log "# Check.Cmd() fs_conference_result:$fs_conference_result"

    if [ ! "$fs_conference_result" == "" ]; then
      print_log "# Check.Cmd() Request timed out.$i"
      timeout=$((timeout+10000))
    else
      print_log "# Check.Cmd() fs_conference_cmd:$fs_conference_cmd"
      break;
    fi
  done

  if [ $(($i)) == 3 ];then
    is_reboot=1
    print_log "# Check.Cmd() Reboot because command timeout."
  fi
fi

# Check Login Interval
#if [ 0 == 1 ]; then
if [ $(($is_reboot)) == 0 ]; then
  # 获取当前时间
  now=$(date +%s)
    
  # 上次登录时间
  login_last_time=$(get_log_datetime "$login_key_line")
  
  print_log "# Check.Login() login_last_time:$login_last_time now:$now log_file_path_last_modify_time:$log_file_path_last_modify_time log_file_path_last:$log_file_path_last"
  # 最新备份的日志最后修改时间为最近900秒, 获取该文件的最后登录时间
  if [ $((login_last_time)) == 0 ] && [ $(($now)) -lt $(($log_file_path_last_modify_time+900)) ];then
    # 上次登录时间
    login_key_line_last_file=$(grep "$login_key" $log_file_path_last | tail -n 1)
    login_last_time=$(get_log_datetime "$login_key_line_last_file")
    
    print_log "# Check.Login().LastFile() login_last_time:$login_last_time login_key_line_last_file:$login_key_line_last_file"
  fi 

  print_log "# Check.Login() login_last_time:$login_last_time now:$now delta_second:$((${now}-${login_last_time}))"
  
  # 是否超过15分钟没有登录
  LOGIN_CHECK_S=900
  if [ $(($now)) -gt $(($login_last_time+$LOGIN_CHECK_S)) ]; then
    print_log "# Reboot because there are no login within 15 minutes."
    is_reboot=1 
  fi
fi

# reboot
#echo "is_reboot: $is_reboot"
if [ $(($is_reboot)) -gt 0 ]; then
  reboot_time=$(date +%Y-%m-%d-%H-%M-%S)
  # print log
  print_log "# Reboot now... "
  
  # Dump thread
  fs_pid=`ps -ef | grep "freeswitch -nc" | grep -v grep | awk '{print $2}'`
  if [ ! $"fs_pid" == "" ];then
    print_log "# Dump thread bt $fs_pid "
    gdb -p $fs_pid -x /usr/local/CamShareServer/dump_thread_bt.init
    mv /tmp/freeswitch.log.dump ${log_folder}/${log_file_name}.$reboot_time.1.dump
  fi
  
  # Stop server
  /usr/local/CamShareServer/stop.sh
  # Change log file name
  mv $log_folder/$log_file_name $log_folder/$log_file_name.$reboot_time.1
  # Start server
  /usr/local/CamShareServer/run.sh
fi
print_log "# Check() End #################################################################################### "