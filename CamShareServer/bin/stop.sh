#!/bin/sh

# CamShareServer Function
CS_CMD="$(ps -ef | grep camshare-middleware | grep -v grep | awk '{print $2}')"
KillCS() {
	PID=$CS_CMD
	if [ -n "$PID" ];then
		echo "kill camshare-middleware"
		`kill $PID`
	fi
}

Kill9CS() {
        PID=$CS_CMD
        if [ -n "$PID" ];then
                echo "kill -9 camshare-middleware"
                `kill -9 $PID`
        fi
}

# Freeswitch Function
FS_CMD="$(ps -ef | grep freeswitch | grep -v grep | awk -F ' ' '{print $2}')"
KillFS() {
	PID=$FS_CMD
	if [ -n "$PID" ];then
		echo "kill freeswitch"
		`kill $PID`
	fi
}

Kill9FS() {
        PID=$FS_CMD
        if [ -n "$PID" ];then
                echo "kill -9 freeswitch"
                `kill -9 $PID`
        fi
}

# Check & Wait
CheckAndWait() {
	for (( i=1; i<5; i++))
	do
	        FS_PID=$FS_CMD
	        CS_PID=$CS_CMD
	        if [ -n "$FS_PID" ] || [ -n "$CS_PID" ]; then
                	sleep 1
        	fi
	done
}

# main code
KillCS
KillFS
CheckAndWait
Kill9CS
Kill9FS
CheckAndWait
