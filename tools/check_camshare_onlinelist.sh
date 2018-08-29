#!/bin/sh

/app/freeswitch/bin/fs_cli -x "rtmp status profile default sessions"| awk -F ',' '{print $3","$5}'|grep -e "[A-Za-z]"
