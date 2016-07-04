#!/bin/sh

nowdt=`date +%Y%m%d%H%M%S`
ffmpeg -i $1 -y -vcodec copy $2-$nowdt.mp4 > /dev/null 2>&1
