#!/bin/sh

# define param
env=$1

# clean package
rm -rf package
mkdir package

# copy config
if [ "$env" = "develop" ]; then
  cp -f ./install/scripts/site_config_develop.lua /usr/local/freeswitch/scripts/site_config.lua
elif [ "$env" = "demo" ]; then
  cp -f ./install/scripts/site_config_demo.lua /usr/local/freeswitch/scripts/site_config.lua
elif [ "$env" = "operating" ]; then
  cp -f ./install/scripts/site_config_operating.lua /usr/local/freeswitch/scripts/site_config.lua
fi

# package freeswitch
cd package
cp -rf /usr/local/freeswitch ./
tar -zcvf freeswitch.tar.gz ./freeswitch
rm -rf ./freeswitch
cd ..
