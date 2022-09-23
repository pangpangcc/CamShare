#!/bin/sh

# 切换到脚本目录
path=$(dirname $0)
cd $path

NOW=`date "+%Y%m%d_%H%M%S"`
VERSION=`cat /app/CamShareServer/version`
DIR="/app/CamShareServer/update/backup_${VERSION}_${NOW}"
mkdir -p ${DIR}

mkdir -p ${DIR}/freeswitch/conf/autoload_configs
cp -f /usr/local/freeswitch/conf/autoload_configs/switch.conf.xml ${DIR}/freeswitch/conf/autoload_configs/

mkdir -p ${DIR}/freeswitch/lib
cp -f /usr/local/freeswitch/lib/libfreeswitch.so.1.0.0 ${DIR}/freeswitch/lib/
cp -f /usr/local/freeswitch/lib/libfreeswitch.la ${DIR}/freeswitch/lib/
cp -f /usr/local/freeswitch/lib/libfreeswitch.a ${DIR}/freeswitch/lib/

mkdir -p ${DIR}/freeswitch/scripts
cp -f /usr/local/freeswitch/scripts/gen_dir_user_xml.lua ${DIR}/freeswitch/scripts/
cp -f /usr/local/freeswitch/scripts/site_config.lua ${DIR}/freeswitch/scripts/

mkdir -p ${DIR}/freeswitch/mod
cp -f /usr/local/freeswitch/mod/mod_conference* ${DIR}/freeswitch/mod/
cp -f /usr/local/freeswitch/mod/mod_file_recorder* ${DIR}/freeswitch/mod/
cp -f /usr/local/freeswitch/mod/mod_rtmp* ${DIR}/freeswitch/mod/
cp -f /usr/local/freeswitch/mod/mod_ws* ${DIR}/freeswitch/mod/

mkdir -p ${DIR}/freeswitch/bin/mod_file_recorder_sh
cp -f /usr/local/freeswitch/bin/mod_file_recorder_sh/close_shell.sh ${DIR}/freeswitch/bin/mod_file_recorder_sh/

mkdir -p ${DIR}/CamShareServer
cp -f /usr/local/CamShareServer/camshare-middleware ${DIR}/CamShareServer/
cp -f /usr/local/CamShareServer/camshare-middleware.config ${DIR}/CamShareServer/
cp -f /usr/local/CamShareServer/check_makecall_fail.sh ${DIR}/CamShareServer/
cp -f /usr/local/CamShareServer/version ${DIR}/CamShareServer/

cp restore.sh ${DIR}

echo "# Camshare update backup ${DIR}"