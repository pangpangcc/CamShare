#!/bin/sh

echo "# copy freeswitch bin ..."
cp -rf ./install/bin/* /usr/local/freeswitch/bin/
chmod +x /usr/local/freeswitch/bin/mod_file_recorder_sh/close_shell.sh
chmod +x /usr/local/freeswitch/bin/mod_file_recorder_sh/pic_shell.sh

echo "# copy freeswitch scripts ..."
cp -rf ./install/scripts/* /usr/local/freeswitch/scripts/

echo "# copy freeswitch conf ..."
cp -rf ./install/conf/* /usr/local/freeswitch/conf/

echo "# create record video folder ..."
mkdir -p /usr/local/freeswitch/recordings
mkdir -p /usr/local/freeswitch/recordings/video_h264
mkdir -p /usr/local/freeswitch/recordings/video_mp4
mkdir -p /usr/local/freeswitch/recordings/pic_h264
mkdir -p /usr/local/freeswitch/recordings/pic_jpg
