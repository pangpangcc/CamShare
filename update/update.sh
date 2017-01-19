#!/bin/sh

# --- stop camshare & freeswitch ---
# stop crontab
systemctl stop crond
# stop camshare & freeswitch
/usr/local/CamShareServer/stop.sh
sleep 5

# 切换到脚本目录
path=$(dirname $0)
cd $path

yum install libpng -y

# ---- freeswitch ----
# freeswitch common config
#cp -f ./file/modules.conf.xml /usr/local/freeswitch/conf/autoload_configs/
cp -f ./file/switch.conf.xml /usr/local/freeswitch/conf/autoload_configs/
#cp -f ./file/vars.xml /usr/local/freeswitch/conf/vars.xml

# freeswitch ./lib & ./mod files
cp -f ./file/lib/* /usr/local/freeswitch/lib/
#cp -f ./file/mod/* /usr/local/freeswitch/mod/

# freeswitch scripts
cp -f ./file/*.lua /usr/local/freeswitch/scripts/

# freeswitch mod_file_recorder shell
#cp -f ./file/close_shell.sh /usr/local/freeswitch/bin/mod_file_recorder_sh/
#cp -f ./file/pic_shell.sh /usr/local/freeswitch/bin/mod_file_recorder_sh/

# freeswitch mod_conference
cp -f ./file/mod_conference* /usr/local/freeswitch/mod/
cp -f ./file/conference.conf.xml /usr/local/freeswitch/conf/autoload_configs/

# freeswitch mod_file_recorder
cp -f ./file/mod_file_recorder* /usr/local/freeswitch/mod/
cp -f ./file/file_recorder.conf.xml /usr/local/freeswitch/conf/autoload_configs/

# freeswitch mod_rtmp
cp -f ./file/mod_rtmp* /usr/local/freeswitch/mod/
#cp -f ./file/rtmp.conf.xml /usr/local/freeswitch/conf/autoload_configs/

# freeswitch mod_ws
cp -f ./file/mod_ws* /usr/local/freeswitch/mod/
#cp -f ./file/ws.conf.xml /usr/local/freeswitch/conf/autoload_configs/

# freeswitch mod_logfile
#cp -f ./file/logfile.conf.xml /usr/local/freeswitch/conf/autoload_configs/

# freeswitch mod_lua
cp -f ./file/lua.conf.xml /usr/local/freeswitch/conf/autoload_configs/

# ---- camshare ----
# camshare-middleware file
cp -f ./file/camshare-middleware /usr/local/CamShareServer/
cp -f ./file/camshare-middleware.config /usr/local/CamShareServer/

# ---- camshare executor ----
cp -f ./file/camshare-executor /usr/local/CamShareServer/
cp -f ./file/camshare-executor.config /usr/local/CamShareServer/

# camshare shell
cp -f ./file/run.sh /usr/local/CamShareServer/
cp -f ./file/stop.sh /usr/local/CamShareServer/
cp -f ./file/check_run.sh /usr/local/CamShareServer/
#cp -f ./file/dump_crash_log.sh /usr/local/CamShareServer/

# camshare clean shell
#cp -rf ./file/clean /usr/local/CamShareServer/

# version
cp -f ./version /usr/local/CamShareServer/

# --- start camshare & freeswitch ---
# run camshare & freeswitch
/usr/local/CamShareServer/run.sh
# start crontab
systemctl start crond
