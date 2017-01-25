#!/bin/sh

# -- CamShareServer Function
GetCamSharePID() {
  CS_PID=`ps -ef | grep camshare-middleware | grep -v grep | awk '{print $2}'`
}

KillCS() {
  GetCamSharePID
  if [ -n "$CS_PID" ]; then
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

GetCamShareExecutorPID() {
  CSEXC_PID=`ps -ef | grep camshare-executor | grep -v grep | awk '{print $2}'`
}

KillCSEXC() {
  GetCamShareExecutorPID
  if [ -n "$CS_PID" ];then
    KILL_CMD="kill $CSEXC_PID"
    echo "kill camshare-executor cmd:'$KILL_CMD'"
    eval $KILL_CMD
  fi
}

Kill9CSEXC() {
  GetCamShareExecutorPID
  if [ -n "$CSEXC_PID" ];then
    KILL_CMD="kill -9 $CSEXC_PID"
    echo "kill -9 camshare-executor cmd:'$KILL_CMD'"
    eval $KILL_CMD
  fi
}

# -- Freeswitch Function
ShutdownFreeswitch() {
  fs_cmd_shutdown=`/usr/local/freeswitch/bin/fs_cli -t 20 -T 20 -x "shutdown"`
  echo "fs_cmd_shutdown : $fs_cmd_shutdown"
}

GetFreeswitchPID() {
	FS_PID=`ps -ef | grep "freeswitch -nc" | grep -v grep | awk '{print $2}'`
}

KillFS() {
#  ShutdownFreeswitch
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
  for (( i=1; i<30; i++))
  do
    GetCamSharePID
    GetFreeswitchPID
    GetCamShareExecutorPID
    #echo "CheckAndWait time : $i"
    
    wait=0
    #if [ "$fs_cmd_shutdown" == "+OK" ] && [ -n "$FS_PID" ]; then
    if [ -n "$FS_PID" ]; then
      # freeswitch服务未停止, 等待
      wait=1
    fi
    
    if [ -n "$CS_PID" ] || [ -n "$CSEXC_PID" ]; then
      # camshare, 服务未停止, 等待
      wait=1
    fi
    
    if [ "$wait" == 1 ]; then
      # 等待
      echo "# waitting..."
      sleep 1
    else
      #echo "CheckAndWait break"
      sleep 1
      break
    fi
    
  done
}

# main code
echo "# Stop all camshare service"
KillCS
KillCSEXC
KillFS
CheckAndWait
Kill9CS
Kill9CSEXC
Kill9FS
sleep 1
echo "# Stop all camshare service OK"
