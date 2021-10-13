#!/bin/sh
# Check Camshare H5 Lady
# Author:	Max.Chiu

NOW=$(date +%s)
DAYS=14
((START=$NOW-($DAYS*3600)))
START_DATE_MONTH=$(date -d @$START +%Y%m)
END_DATE_MONTH=$(date -d @$NOW +%Y%m)
LOG_FILE="/app/live/mediaserver/log/mediaserver/info/Log${START_DATE_MONTH}*"
if [ ! "$START_DATE_MONTH" == "$END_DATE_MONTH" ];then
  LOG_FILE="$LOG_FILE /app/live/mediaserver/log/mediaserver/info/Log$END_DATE_MONTH*"
fi
echo -e "[\033[32mLOG_FILE:\033[0m] $LOG_FILE"

function check_lady_list() {
  RECORD=$(grep 推流 $LOG_FILE | awk -F 'uid=|\\&' '{print $2}' | grep -v CM | sort | uniq)
  echo "$RECORD"
}

function check_lady() {
  WOMANID=$1

  RECORD=$(grep ${WOMANID} $LOG_FILE | grep 推流 | awk -F ':' '{print $10}' | awk -F ',' '{print $1}' | xargs -I {} grep {} $LOG_FILE | grep OnWSClose)
  echo "$RECORD" | awk -F '\\[ | tid|aliveTime : | )' '{print $2" "$4}'
}

TMP=ladys.tmp
check_lady_list $MONTH > $TMP
while read LINE; do
  echo -e "[\033[32m$LINE\033[0m]"
  check_lady $LINE 
  echo ""
done < $TMP
rm $TMP -f