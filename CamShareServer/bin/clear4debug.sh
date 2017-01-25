#!/bin/sh
rm /usr/local/CamShareServer/log/* -rf
rm /usr/local/CamShareServer/local*.db -rf

rm /usr/local/freeswitch/log/* -rf
rm /usr/local/freeswitch/db* -rf

rm /usr/local/freeswitch/recordings/video_mp4/* -rf
rm /usr/local/freeswitch/recordings/video_h264/* -rf
rm /usr/local/freeswitch/recordings/pic_h264/* -rf
rm /usr/local/freeswitch/recordings/pic_jpg/* -rf