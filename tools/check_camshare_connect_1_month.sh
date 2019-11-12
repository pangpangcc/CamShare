#!/bin/sh
# 统计30天内connect&login数据

TIME=$(date +%s)
DAYS=30
i=0
while [[ $i -lt $DAYS ]];do
        TIME_FORMAT=$(date -d @$TIME +%Y/%m/%d)
        TIME_FORMAT_SHOW=$(date -d @$TIME +%Y/%m/%d:%a)
	
	# 统计
	TIME_FORMAT_LOG=$(date -d @$TIME +%Y-%m-%d)
	TIME_FORMAT_LOG_FILE=$(date -d @$TIME +%Y%m)

	LOGIN_COUNT=$(grep -e "$TIME_FORMAT_LOG" /app/CamShareServer/log/camshare_server/info/Log$TIME_FORMAT_LOG_FILE* | grep "Freeswitch-事件处理-rtmp终端登陆\|Freeswitch-事件处理-websocket终端登录" | wc -l)
	LOGIN_MAN_COUNT=$(grep -e "$TIME_FORMAT_LOG" /app/CamShareServer/log/camshare_server/info/Log$TIME_FORMAT_LOG_FILE* | grep "Freeswitch-事件处理-rtmp终端登陆], user : CM\|Freeswitch-事件处理-websocket终端登录], user : CM" | wc -l)
        MAKECALL_COUNT=$(grep -e "$TIME_FORMAT_LOG" /app/CamShareServer/log/camshare_server/info/Log$TIME_FORMAT_LOG_FILE* | grep "Freeswitch-事件处理-增加会议室成员" | wc -l)
        MAKECALL_MAN_COUNT=$(grep -e "$TIME_FORMAT_LOG" /app/CamShareServer/log/camshare_server/info/Log$TIME_FORMAT_LOG_FILE* | grep "Freeswitch-事件处理-增加会议室成员], user : CM" | wc -l)
        MAKECALL_TIMEOUT_COUNT=$(grep -e "$TIME_FORMAT_LOG" /app/CamShareServer/log/camshare_server/info/Log$TIME_FORMAT_LOG_FILE* | grep "外部服务(LiveChat)-收到命令:进入会议室认证结果-失败-找不到任务-已经超时" | grep "ACTIVE" | wc -l)
        MAKECALL_FAIL_COUT=$(grep -e "$TIME_FORMAT_LOG" /app/CamShareServer/log/camshare_server/info/Log$TIME_FORMAT_LOG_FILE* | grep "外部服务(LiveChat)-收到命令:进入会议室认证结果-失败]" | wc -l)

	printf "$TIME_FORMAT_SHOW login: \033[32m%5s\033[0m, man login: \033[33m%5s\033[0m, makecall: \033[36m%5s\033[0m, man makecall: \033[34m%5s\033[0m, makecall timeout: \033[35m%5s\033[0m, makecall fail: \033[31m%5s\033[0m \n" $LOGIN_COUNT $LOGIN_MAN_COUNT $MAKECALL_COUNT $MAKECALL_MAN_COUNT $MAKECALL_TIMEOUT_COUNT $MAKECALL_FAIL_COUT
	
	((i++))
	((TIME=$TIME-(24*3600)))
	
done
