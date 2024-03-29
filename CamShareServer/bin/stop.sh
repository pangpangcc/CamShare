#!/bin/sh

# -- CamShareServer Function
GetCamSharePID() {
	CS_PID=`ps -ef | grep camshare-middleware | grep -v grep | awk '{print $2}'`
}

KillCS() {
	GetCamSharePID
	if [ -n "$CS_PID" ];then
                KILL_CMD="kill $CS_PID"
		echo "kill camshare-middleware cmd:'$KILL_CMD'"
		eval $KILL_CMD
	fi
}

Kill9CS() {
	GetCamSharePID
        if [ -n "$CS_PID" ];then
		KILL_CMD="kill -9 $CS_PID"
                echo "kill -9 camshare-middleware cmd:'$KILL_CMD'"
                eval $KILL_CMD
        fi
}

# -- Freeswitch Function
GetFreeswitchPID() {
	FS_PID=`ps -ef | grep freeswitch | grep -v grep | awk '{print $2}'`
}

KillFS() {
	GetFreeswitchPID
	if [ -n "$FS_PID" ];then
		KILL_CMD="kill $FS_PID"
		echo "kill freeswitch cmd:'$KILL_CMD'"
		eval $KILL_CMD
	fi
}

Kill9FS() {
	GetFreeswitchPID
        if [ -n "$FS_PID" ];then
		KILL_CMD="kill -9 $FS_PID"
                echo "kill -9 freeswitch cmd:'$KILL_CMD'"
                eval $KILL_CMD
        fi
}

# -- Check & Wait
CheckAndWait() {
	for (( i=1; i<5; i++))
	do
	        GetCamSharePID
		GetFreeswitchPID
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
