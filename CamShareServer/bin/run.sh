#!/bin/sh

echo "# Run all camshare service"

# 定义目录路径
CAMSHARE_FOLDER="/usr/local/CamShareServer"
FREESWITCH_FOLDER="/usr/local/freeswitch/bin"

# 开放权限
$CAMSHARE_FOLDER/ulimit.sh

# 创建日志目录
CAMSHARE_LOG_FOLDER="$CAMSHARE_FOLDER/log"
if [ ! -e $CAMSHARE_LOG_FOLDER ]; then
  mkdir $CAMSHARE_LOG_FOLDER -p
fi

# 检测 camshare-middlewarecd 是否运行中
CAMSHARE_MIDDLEWARE="camshare-middleware"
CS_MIDDLE_PID=$(ps -ef | grep $CAMSHARE_MIDDLEWARE | grep -v "grep" | awk '{ print $2 }')
if [ -n "$CS_MIDDLE_PID" ]; then
  echo "$CAMSHARE_MIDDLEWARE is running, please stop first."
  exit 1
fi

# 检测 camshare-executor 是否运行中
CAMSHARE_EXECUTOR="camshare-executor"
CS_EXECUTOR_PID=$(ps -ef | grep $CAMSHARE_EXECUTOR | grep -v "grep" | awk '{ print $2 }')
if [ -n "$CS_EXECUTOR_PID" ]; then
  echo "$CAMSHARE_EXECUTOR is running, please stop first."
  exit 1
fi

# 检测 freeswitch 是否运行中
FREESWITCH="freeswitch"
FREESWITCH_PID=$(ps -ef | grep "$FREESWITCH -nc" | grep -v "grep" | awk '{ print $2 }')
if [ -n "$FREESWITCH_PID" ]; then
  echo "$FREESWITCH is running, please stop first."
  exit 1
fi

# 启动 camshare-middleware
cd $CAMSHARE_FOLDER
nohup $CAMSHARE_FOLDER/camshare-middleware -f $CAMSHARE_FOLDER/camshare-middleware.config 2>&1>/dev/null &
# 启动 camshare-executor
nohup $CAMSHARE_FOLDER/$CAMSHARE_EXECUTOR -f $CAMSHARE_FOLDER/camshare-executor.config 2>&1>/dev/null &
cd -
# 启动 freeswitch
cd $FREESWITCH_FOLDER
$FREESWITCH_FOLDER/freeswitch -nc
cd -

sleep 3

# 打印 log
echo "# $CAMSHARE_MIDDLEWARE pid : `ps -ef | grep $CAMSHARE_MIDDLEWARE | grep -v "grep" | awk '{ print $2 }'`"
echo "# camshare-executor pid : `ps -ef | grep camshare-executor | grep -v "grep" | awk -F" " '{ print $2 }'`"
echo "# freeswitch pid : `ps -ef | grep "freeswitch -nc" | grep -v "grep" | awk -F" " '{ print $2 }'`"
echo "# Run all camshare service OK"
