#!/bin/sh
PID=`ps -ef | grep camshare-middleware | grep -v "grep" | awk -F" " '{ print $2 }'`
if [ -n "$PID" ];then
	echo "kill camshare-middleware"
	`kill -9 $PID`
fi

PID=`ps -ef | grep freeswitch | grep -v "grep" | awk -F" " '{ print $2 }'`
if [ -n "$PID" ];then
	echo "kill freeswitch"
	`kill -9 $PID`
fi
#killall -9 camshare-middleware
#killall -9 freeswitch

