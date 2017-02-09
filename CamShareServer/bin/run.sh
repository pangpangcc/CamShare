#!/bin/sh

echo "# Run all camshare service"

# 增加权限
#chmod +x /usr/local/CamShareServer/camshare-middleware
#chmod +x /usr/local/freeswitch/bin/freeswitch
#chmod +x /usr/local/freeswitch/bin/fs_cli

# 停止程序
#/usr/local/CamShareServer/stop.sh

# 开放权限
/usr/local/CamShareServer/ulimit.sh

# 创建日志目录
mkdir /usr/local/CamShareServer/log -p

# 运行camshare-middlewarecd
cd /usr/local/CamShareServer/ && nohup ./camshare-middleware -f ./camshare-middleware.config 2>&1>/dev/null &

# 运行camshare-executor
cd /usr/local/CamShareServer/ && nohup ./camshare-executor -f ./camshare-executor.config 2>&1>/dev/null &

# 运行freeswitch
cd /usr/local/freeswitch/bin && ./freeswitch -nc

echo "# camshare-middlewarecd pid : `ps -ef | grep camshare-middleware | grep -v "grep" | awk -F" " '{ print $2 }'`"
echo "# camshare-executor pid : `ps -ef | grep camshare-executor | grep -v "grep" | awk -F" " '{ print $2 }'`"
echo "# freeswitch pid : `ps -ef | grep "freeswitch -nc" | grep -v "grep" | awk -F" " '{ print $2 }'`"

echo "# Run all camshare service OK"
