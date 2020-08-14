#!/bin/sh

# define param
env=$1
ver=$2 

# ---- print param error
param_err="0"
if [ "$env" != "develop" ] && [ "$env" != "demo" ] && [ "$env" != "operating" ]; then
  param_err="1"
elif [ -z "$ver" ]; then
  param_err="2"
fi
if [ "$param_err" != "0" ]; then
  echo "$0 [develop | demo | operating] version"
  exit 0
fi

# ---- define dir
updatedir="./update"
updatefiledir="$updatedir/file"
updatepackagedir="./update_package"
updatepackagepath="$updatepackagedir/camshare_update_$env.tar.gz"

# ---- clean dir & make dir
if [ -e $updatepackagepath ]; then
  rm -f $updatepackagepath
fi
rm -rf $updatefiledir/*
mkdir -p $updatedir/file

# ---- make version file
echo "$ver" > $updatedir/version

# ---- set update.sh executable
chmod +x $updatedir/update.sh

# ---- copy freeswitch files
# copy all ./lib & ./mod files
#mkdir -p $updatefiledir/bin
#mkdir -p $updatefiledir/lib
#cp -f /usr/local/freeswitch/lib/* $updatefiledir/lib/
# copy common lib
#cp -f /usr/local/freeswitch/bin/freeswitch $updatefiledir/bin/
#cp -f /usr/local/freeswitch/lib/libfreeswitch.a $updatefiledir/lib/
#cp -f /usr/local/freeswitch/lib/libfreeswitch.la $updatefiledir/lib/
#cp -f /usr/local/freeswitch/lib/libfreeswitch.so.1.0.0 $updatefiledir/lib/

#mkdir -p $updatefiledir/mod
#cp -f /usr/local/freeswitch/mod/* $updatefiledir/mod/

# copy common configs
#cp -f /usr/local/freeswitch/conf/autoload_configs/modules.conf.xml $updatefiledir/
#cp -f /usr/local/freeswitch/conf/autoload_configs/switch.conf.xml $updatefiledir/
#cp -f /usr/local/freeswitch/conf/vars.xml $updatefiledir/

# copy lua configs
#cp -f /usr/local/freeswitch/conf/autoload_configs/lua.conf.xml $updatefiledir/

# copy shell
#cp -f /usr/local/freeswitch/bin/mod_file_recorder_sh/close_shell.sh $updatefiledir/
#cp -f /usr/local/freeswitch/bin/mod_file_recorder_sh/pic_shell.sh $updatefiledir/

# copy mod_conference files
cp -f /usr/local/freeswitch/mod/mod_conference.so $updatefiledir/
cp -f /usr/local/freeswitch/mod/mod_conference.la $updatefiledir/
#cp -f /usr/local/freeswitch/conf/autoload_configs/conference.conf.xml $updatefiledir/

# copy mod_file_recorder files
cp -f /usr/local/freeswitch/mod/mod_file_recorder.so $updatefiledir/
cp -f /usr/local/freeswitch/mod/mod_file_recorder.la $updatefiledir/
#cp -f /usr/local/freeswitch/conf/autoload_configs/file_recorder.conf.xml $updatefiledir/

# copy mod_rtmp files
cp -f /usr/local/freeswitch/mod/mod_rtmp.so $updatefiledir/
cp -f /usr/local/freeswitch/mod/mod_rtmp.la $updatefiledir/
#cp -f /usr/local/freeswitch/conf/autoload_configs/rtmp.conf.xml $updatefiledir/

# copy mod_ws files
#cp -f /usr/local/freeswitch/mod/mod_ws.so $updatefiledir/
#cp -f /usr/local/freeswitch/mod/mod_ws.la $updatefiledir/
#cp -f /usr/local/freeswitch/conf/autoload_configs/ws.conf.xml $updatefiledir/

# copy mod_logfile files
#cp -f /usr/local/freeswitch/conf/autoload_configs/logfile.conf.xml $updatefiledir/

# copy freeswitch scripts
#cp -f ./freeswitch/install/scripts/common.lua $updatefiledir/
#cp -f ./freeswitch/install/scripts/dialplan_internal_default.lua $updatefiledir/
cp -f ./freeswitch/install/scripts/gen_dir_user_xml.lua $updatefiledir/
#cp -f ./freeswitch/install/scripts/event_file_recorder.lua $updatefiledir/
#cp -f ./freeswitch/install/scripts/event_conference.lua $updatefiledir/

# remove freeswitch scripts config
#rm -f $updatefiledir/site_config*.lua
#if [ "$env" == "develop" ]; then
#  cp -f ./freeswitch/install/scripts/site_config_develop.lua $updatefiledir/site_config.lua
#elif [ "$env" == "demo" ]; then
#  cp -f ./freeswitch/install/scripts/site_config_demo.lua $updatefiledir/site_config.lua
#elif [ "$env" == "operating" ]; then
#  cp -f ./freeswitch/install/scripts/site_config_operating.lua $updatefiledir/site_config.lua
#fi

# ---- copy camshare files
# copy camshare-middleware file 
cp -f ./CamShareServer/camshare-middleware $updatefiledir/

# copy configure file
#if [ "$env" == "develop" ]; then
#  cp -f ./CamShareServer/camshare-middleware.config.develop $updatefiledir/camshare-middleware.config
#elif [ "$env" == "demo" ]; then
#  cp -f ./CamShareServer/camshare-middleware.config.demo $updatefiledir/camshare-middleware.config
#elif [ "$env" == "operating" ]; then
#  cp -f ./CamShareServer/camshare-middleware.config.operating $updatefiledir/camshare-middleware.config
#fi

# ---- copy camshare executor files
cp -f ./CamShareServer/executor/camshare-executor $updatefiledir/
#cp -f ./CamShareServer/executor/camshare-executor.config $updatefiledir/

# copy camshare shell
cp -f ./CamShareServer/bin/run.sh $updatefiledir/
#cp -f ./CamShareServer/bin/stop.sh $updatefiledir/
#cp -f ./CamShareServer/bin/check_run.sh $updatefiledir/
#cp -f ./CamShareServer/bin/dump_crash_log.sh $updatefiledir/
#cp -f ./CamShareServer/bin/check.sh $updatefiledir/
#cp -f ./CamShareServer/bin/check_makecall_fail.sh $updatefiledir/

# copy camshare clean shell
#cp -rf ./CamShareServer/bin/clean $updatefiledir/
#mkdir -p $updatefiledir/clean
#cp -rf ./CamShareServer/bin/clean/clean_h264.sh $updatefiledir/clean/
#cp -rf ./CamShareServer/bin/clean/backup_log.sh $updatefiledir/clean/
#cp -rf ./CamShareServer/bin/clean/clean_log.sh $updatefiledir/clean/

# ---- copy deps build file
#mkdir -p $updatefiledir/usr_local_bin
#cp -f ./deps/build/bin/* $updatefiledir/usr_local_bin/

# build package
mkdir -p $updatepackagedir
rm -f $updatepackagepath
tar zcvf $updatepackagepath $updatedir/
