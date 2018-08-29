#!/bin/sh
# 统计1天内connect&login数据

# initial
TARGET_USERID=$1
TARGET_START_DATE=$2
TARGET_END_DATE=$3
if [ -z "$TARGET_START_DATE" ] || [ -z "$TARGET_USERID" ]; then
  echo "check_camshare_connect_1_day.sh <userid> <target start date[yyyy-mm-dd]>"
  exit 0
fi

if [ -z "$TARGET_END_DATE" ]; then
  TARGET_END_DATE=$TARGET_START_DATE
fi


# loop target date
TARGET_START_TIME=$(date -d "$TARGET_START_DATE 00:00:00" +%s)
TARGET_END_TIME=$(date -d "$TARGET_END_DATE 00:00:00" +%s)
while [[ $TARGET_START_TIME -le $TARGET_END_TIME ]]; do
       	TIME_FORMAT=$(date -d "$TARGET_START_DATE" "+%Y/%m/%d")

	START_DATE=-1
	((MAX_DATE=3+$START_DATE))
	((LOG_FILE_TIME=$TARGET_START_TIME+$START_DATE*24*3600))

	while [[ $START_DATE -lt $MAX_DATE ]]; do
		# log time		
		TIME_FORMAT_LOG=$(date -d @$TARGET_START_TIME "+%Y-%m-%d")
		
		# log file
		TIME_FORMAT_LOG_FILE=$(date -d @$LOG_FILE_TIME "+%Y%m%d")

                # combine Login condition
                LOGIN_KEY="Freeswitch-事件处理-rtmp终端登陆"
                LOGIN_CONDITION_REX=""
                LOGIN_CONDITION_REX=$LOGIN_CONDITION_REX" -e \"$LOGIN_KEY.*$TARGET_USERID\""
                LOGIN_WEBSOCKET_KEY="Freeswitch-事件处理-websocket终端登录"
                LOGIN_CONDITION_REX=$LOGIN_CONDITION_REX" -e \"$LOGIN_WEBSOCKET_KEY.*$TARGET_USERID\""

                # combine Logout condition
                LOGOUT_KEY="Freeswitch-事件处理-rtmp终端断开-有身份"
                LOGOUT_CONDITION_REX=""
                LOGOUT_CONDITION_REX=$LOGOUT_CONDITION_REX" -e \"$LOGOUT_KEY.*$TARGET_USERID\""
                LOGOUT_WEBSOCKET_KEY="Freeswitch-事件处理-websocket终端断开"
                LOGOUT_CONDITION_REX=$LOGOUT_CONDITION_REX" -e \"$LOGOUT_WEBSOCKET_KEY.*$TARGET_USERID\""

                # combine MakeCall condition
                MAKECALL_KEY="Freeswitch-事件处理-增加会议室成员], user : "
                MAKECALL_KEY2=", conference : "
                MAKECALL_CONDITION_REX=""
                MAKECALL_CONDITION_REX=$MAKECALL_CONDITION_REX" -e \"$MAKECALL_KEY$TARGET_USERID$MAKECALL_KEY2\""

                # combine Hangup condition
                HANGUP_KEY="Freeswitch-事件处理-删除会议室成员], user : "
                HANGUP_KEY2=", conference : "
                HANGUP_CONDITION_REX=""
                HANGUP_CONDITION_REX=$HANGUP_CONDITION_REX" -e \"$HANGUP_KEY$TARGET_USERID$HANGUP_KEY2\""

                # combine fail condition
                ENTER_FAIL_KEY="外部服务(LiveChat)-收到命令:进入会议室认证结果-失败"
                ENTER_FAIL_CONDITION_REX=""
                ENTER_FAIL_CONDITION_REX=$ENTER_FAIL_CONDITION_REX" -e \"$ENTER_FAIL_KEY.*, fromId : '$TARGET_USERID.*, toId : '\""
                KICKOFF_KEY="外部服务(LiveChat)-收到命令:从会议室踢出用户-成功"
                KICKOFF_CONDITION_REX=""
                KICKOFF_CONDITION_REX=$KICKOFF_CONDITION_REX" -e \"$KICKOFF_KEY.*, fromId : '$TARGET_USERID.*, toId : '\""

                # grep log
                ALL_CONDITION_REX="$LOGIN_CONDITION_REX$LOGOUT_CONDITION_REX$MAKECALL_CONDITION_REX$HANGUP_CONDITION_REX$ENTER_FAIL_CONDITION_REX$KICKOFF_CONDITION_REX"
                COMMAND_STR="grep -s -h $TIME_FORMAT_LOG /app/CamShareServer/log/camshare_server/info/Log$TIME_FORMAT_LOG_FILE* | grep  $ALL_CONDITION_REX"
                eval $COMMAND_STR

		((LOG_FILE_TIME=$LOG_FILE_TIME+24*3600))
		((START_DATE++))
	done


	((TARGET_START_TIME=$TARGET_START_TIME+24*3600))
done
