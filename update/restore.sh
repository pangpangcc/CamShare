#!/bin/sh

# 切换到脚本目录
path=$(dirname $0)
cd $path

# --- stop camshare & freeswitch ---
# stop crontab
systemctl stop crond
# stop camshare & freeswitch
/usr/local/CamShareServer/stop.sh
sleep 5

OLD_VERSION=`cat /app/CamShareServer/version`

cp -rf CamShareServer/* /app/CamShareServer/
cp -rf freeswitch/* /app/freeswitch/

VERSION=`cat /app/CamShareServer/version`

echo "# Camshare update restore ${OLD_VERSION} => ${VERSION}"

# --- start camshare & freeswitch ---
# run camshare & freeswitch
/usr/local/CamShareServer/run.sh
# start crontab
systemctl start crond