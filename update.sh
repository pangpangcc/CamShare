#!/bin/sh

# define param
env=$1 

# print param error
if [ "$env" != "develop" ] && [ "$env" != "demo" ] && [ "$env" != "operating" ]; then
  echo "$0 [develop | demo | operating]"
  exit 0
fi

# define dir
updatedir="./update"
updatefiledir="$updatedir/file"
updatepackagedir="./update_package"
updatepackagepath="$updatepackagedir/freeswitch_update_$env.tar.gz"

# clean dir & make dir
rm -f freeswitch_update.tar.gz
rm -f $updatefiledir/*
mkdir -p $updatedir/file

# copy freeswitch files
cp -f /usr/local/freeswitch/bin/mod_file_recorder_sh/close_shell.sh $updatefiledir/
cp -f /usr/local/freeswitch/mod/mod_file_recorder.so $updatefiledir/
cp -f /usr/local/freeswitch/mod/mod_file_recorder.la $updatefiledir/

# copy camshare files
if [ "$env" == "develop" ]; then
  cp -f ./CamShareServer/camshare-middleware.config.develop $updatefiledir/camshare-middleware.config
elif [ "$env" == "demo" ]; then
  cp -f ./CamShareServer/camshare-middleware.config.demo $updatefiledir/camshare-middleware.config
elif [ "$env" == "operating" ]; then
  cp -f ./CamShareServer/camshare-middleware.config.operating $updatefiledir/camshare-middleware.config
fi
cp -f ./CamShareServer/camshare-middleware $updatefiledir/

# build package
mkdir -p $updatepackagedir
rm -f $updatepackagepath
tar zcvf $updatepackagepath $updatedir/
