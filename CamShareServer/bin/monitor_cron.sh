#!/bin/sh

# 切换到脚本目录
path=$(dirname $0)
cd $path

function sendmail()
{
	echo 'Freeswitch is down' | mail -s "Freeswitch is down" -t max.chiu@qpidnetwork.com -a From:max.chiu@qpidnetwork.com
}

function main()
{
	pid=`ps -ef | grep fs_cli | grep -v grep | awk -F ' ' '{print $2}'`
	if [ ! "$pid" == "" ]; then
	  echo "Send Mail"
	  #sendmail()
	fi
	fs_conference_list=`/usr/local/freeswitch/bin/fs_cli -x "conference list"`
	echo "$fs_conference_list"
}

# runing
main
