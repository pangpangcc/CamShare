#!/bin/sh

# get current path
curr_path=`pwd`

# define CamShareServer log path
cs_log_folder="/usr/local/CamShareServer/log"
cs_bak_time="90"
cs_bak_limit_time="120"

# define file name and path
cs_exe_name=camshare_executor
cs_svr_name=camshare_server
cs_log_file="Log*.txt"
cs_log_backup_folder="$cs_log_folder/info_bak"

# backup log file
function backup_log_file()
{
  app_name=$1
  cs_app_backup_folder=$cs_log_backup_folder/$app_name
  cs_log_path=$cs_log_folder/$app_name/info
  cs_old_num=`find $cs_log_path -mtime +$cs_bak_limit_time -name "$cs_log_file" | wc -l`
  ((cs_old_file_num=10#$cs_old_num))
  if [ $cs_old_file_num -gt 0 ]; then
    # create backup folder
    now_time=`date --date="$cs_bak_time days ago" +%Y%m%d%H%M%S`
    backup_folder="$cs_app_backup_folder/Log$now_time"
    mkdir -p $backup_folder
  
    # find and move old file to backup folder
    find $cs_log_path -mtime +$cs_bak_time -name "$cs_log_file" -exec mv -t $backup_folder {} \;
  
    # packet backup log file
    cd $backup_folder
    backup_file=$backup_folder".tar.gz"
    tar zcvf $backup_file ./*
    cd ..
  
    # clean backup folder
    rm -rf $backup_folder

    # back to path
    cd $curr_path
  fi
}

# backup log file
backup_log_file $cs_exe_name
backup_log_file $cs_svr_name

