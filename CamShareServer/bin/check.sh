# !/bin/sh

# define folder path
camshare_folder="/usr/local/CamShareServer"

# define log file path
log_folder="$camshare_folder/log"
log_file_path="$log_folder/check_run.log"
log_file_check_makecall_path="$log_folder/check_makecall.log"

# make folder
if [ ! -e $log_folder ]; then
  mkdir -p $log_folder
fi

# check 
$camshare_folder/check_run.sh $log_file_path || exit 1
$camshare_folder/check_makecall_fail.sh $log_file_check_makecall_path || exit 1