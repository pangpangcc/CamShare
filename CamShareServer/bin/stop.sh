#!/bin/sh
PID=`ps -ef | grep camshare-middleware | grep -v "grep" | awk -F" " '{ print $2 }'`
if [ -n "$PID" ];then
	echo "kill camshare-middleware"
	`kill $PID`
fi

PID=`ps -ef | grep freeswitch | grep -v "grep" | awk -F" " '{ print $2 }'`
if [ -n "$PID" ];then
	echo "kill freeswitch"
	`kill $PID`
fi
#killall camshare-middleware
#killall freeswitch

