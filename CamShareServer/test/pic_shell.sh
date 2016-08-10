#!/bin/sh

ffmpeg -i $1 -y -s 240x180 -vframes 1 $2 > /dev/null 2>&1
