#!/bin/sh

# define param
env=$1 

# ---- print param error
if [ "$env" != "develop" ] && [ "$env" != "demo" ] && [ "$env" != "operating" ]; then
  echo "$0 [develop | demo | operating]"
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

# ---- set update.sh executable
chmod +x $updatedir/update.sh

# ---- copy freeswitch files
# copy all ./lib & ./mod files
#mkdir -p $updatefiledir/mod
#cp -f /usr/local/freeswitch/mod/* $updatefiledir/mod/
#mkdir -p $updatefiledir/lib
#cp -f /usr/local/freeswitch/lib/* $updatefiledir/lib/

# copy shell
#cp -f /usr/local/freeswitch/bin/mod_file_recorder_sh/close_shell.sh $updatefiledir/
#cp -f /usr/local/freeswitch/bin/mod_file_recorder_sh/pic_shell.sh $updatefiledir/

# copy mod_file_recorder files
#cp -f /usr/local/freeswitch/mod/mod_file_recorder.so $updatefiledir/
#cp -f /usr/local/freeswitch/mod/mod_file_recorder.la $updatefiledir/
#cp -f /usr/local/freeswitch/conf/autoload_configs/file_recorder.conf.xml $upatefiledir/

# copy mod_rtmp files
cp -f /usr/local/freeswitch/mod/mod_rtmp.so $updatefiledir/
cp -f /usr/local/freeswitch/mod/mod_rtmp.la $updatefiledir/

# copy freeswitch scripts
#cp -f ./freeswitch/install/scripts/* $updatefiledir/

# remove freeswitch scripts config
#rm -f $updatefiledir/site_config_develop.lua
#rm -f $updatefiledir/site_config_demo.lua
#rm -f $updatefiledir/site_config_operating.lua
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
if [ "$env" == "develop" ]; then
  cp -f ./CamShareServer/camshare-middleware.config.develop $updatefiledir/camshare-middleware.config
elif [ "$env" == "demo" ]; then
  cp -f ./CamShareServer/camshare-middleware.config.demo $updatefiledir/camshare-middleware.config
elif [ "$env" == "operating" ]; then
  cp -f ./CamShareServer/camshare-middleware.config.operating $updatefiledir/camshare-middleware.config
fi

# copy camshare shell
#cp -f ./CamShareServer/bin/run.sh $updatefiledir/
cp -f ./CamShareServer/bin/stop.sh $updatefiledir/
#cp -f ./CamShareServer/bin/check_run.sh $updatefiledir/
cp -f ./CamShareServer/bin/dump_crash_log.sh $updatefiledir/

# build package
mkdir -p $updatepackagedir
rm -f $updatepackagepath
tar zcvf $updatepackagepath $updatedir/
