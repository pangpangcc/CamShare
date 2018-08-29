#!/bin/sh
# 统计1天内connect&login数据

# initial
TARGET_START_DATE=$1
if [ -z "$TARGET_START_DATE" ]; then
  echo "check_camshare_connect_1_day.sh <target start date[yyyy-mm-dd]> [target end date]"
  exit 0
fi

TARGET_END_DATE=$2
if [ -z "$TARGET_END_DATE" ];then
  TARGET_END_DATE=$TARGET_START_DATE
fi

# loop target date
TARGET_START_TIME=$(date -d "$TARGET_START_DATE 00:00:00" +%s)
TARGET_END_TIME=$(date -d "$TARGET_END_DATE 00:00:00" +%s)
while [[ $TARGET_START_TIME -le $TARGET_END_TIME ]]; do
	TIME=$TARGET_START_TIME
	START_TIME=$TIME
	START_HOUR=0
	while [[ $START_HOUR -lt 24 ]];do
        	TIME_FORMAT=$(date -d @$START_TIME "+%Y/%m/%d")
	        TIME_FORMAT_SHOW=$(date -d @$START_TIME "+%a %Y/%m/%d %H:%M")
	
		# 统计
		LOGIN_COUNT=0
		LOGIN_MAN_COUNT=0
		MAKECALL_COUNT=0
		MAKECALL_MAN_COUNT=0
		MAKECALL_TIMEOUT_COUNT=0
		MAKECALL_FAIL_COUNT=0

		START_DATE=-1
		((MAX_DATE=3-$START_DATE))
		((LOG_FILE_TIME=$TIME+$START_DATE*24*3600))
		while [[ $START_DATE -lt $MAX_DATE ]]; do
			# log time		
			TIME_FORMAT_LOG=$(date -d @$START_TIME "+%Y-%m-%d %H")
		
			# log file
			TIME_FORMAT_LOG_FILE=$(date -d @$LOG_FILE_TIME "+%Y%m%d")

			THE_LOGIN_COUNT=$(grep -s -e "$TIME_FORMAT_LOG" /app/CamShareServer/log/camshare_server/info/Log$TIME_FORMAT_LOG_FILE* | grep "Freeswitch-事件处理-rtmp终端登陆\|Freeswitch-事件处理-websocket终端登录" | wc -l)
			((LOGIN_COUNT=$LOGIN_COUNT+$THE_LOGIN_COUNT))

			THE_LOGIN_MAN_COUNT=$(grep -s -e "$TIME_FORMAT_LOG" /app/CamShareServer/log/camshare_server/info/Log$TIME_FORMAT_LOG_FILE* | grep "Freeswitch-事件处理-rtmp终端登陆], user : CM\|Freeswitch-事件处理-websocket终端登录], user : CM" | wc -l)
			((LOGIN_MAN_COUNT=$LOGIN_MAN_COUNT+$THE_LOGIN_MAN_COUNT))

		        THE_MAKECALL_COUNT=$(grep -s -e "$TIME_FORMAT_LOG" /app/CamShareServer/log/camshare_server/info/Log$TIME_FORMAT_LOG_FILE* | grep "Freeswitch-事件处理-增加会议室成员" | wc -l)
			((MAKECALL_COUNT=$MAKECALL_COUNT+$THE_MAKECALL_COUNT))

			THE_MAKECALL_MAN_COUNT=$(grep -s -e "$TIME_FORMAT_LOG" /app/CamShareServer/log/camshare_server/info/Log$TIME_FORMAT_LOG_FILE* | grep "Freeswitch-事件处理-增加会议室成员], user : CM" | wc -l)
			((MAKECALL_MAN_COUNT=$MAKECALL_MAN_COUNT+$THE_MAKECALL_MAN_COUNT))

			THE_MAKECALL_TIMEOUT_COUNT=$(grep -s -e "$TIME_FORMAT_LOG" /app/CamShareServer/log/camshare_server/info/Log$TIME_FORMAT_LOG_FILE* | grep "外部服务(LiveChat)-收到命令:进入会议室认证结果-失败-找不到任务-已经超时" | grep "ACTIVE" | wc -l)
			((MAKECALL_TIMEOUT_COUNT=$MAKECALL_TIMEOUT_COUNT+$THE_MAKECALL_TIMEOUT_COUNT))

		        THE_MAKECALL_FAIL_COUNT=$(grep -s -e "$TIME_FORMAT_LOG" /app/CamShareServer/log/camshare_server/info/Log$TIME_FORMAT_LOG_FILE* | grep "外部服务(LiveChat)-收到命令:进入会议室认证结果-失败" | wc -l)
			((MAKECALL_FAIL_COUNT=$MAKECALL_FAIL_COUNT+$THE_MAKECALL_FAIL_COUNT))

			((LOG_FILE_TIME=$LOG_FILE_TIME+24*3600))
			((START_DATE++))
		done

		printf "$TIME_FORMAT_SHOW login: \033[32m%5s\033[0m, man login: \033[33m%5s\033[0m, makecall: \033[36m%5s\033[0m, man makecall: \033[34m%5s\033[0m, makecall timeout: \033[35m%5s\033[0m, makecall fail: \033[31m%5s\033[0m \n" $LOGIN_COUNT $LOGIN_MAN_COUNT $MAKECALL_COUNT $MAKECALL_MAN_COUNT $MAKECALL_TIMEOUT_COUNT $MAKECALL_FAIL_COUNT

		((START_TIME=$START_TIME+3600))
		((START_HOUR++))
	done
	
	((TARGET_START_TIME=$TARGET_START_TIME+24*3600))
done
