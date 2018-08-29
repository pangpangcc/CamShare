#!/bin/sh

if [ "-$1" = "-" ]; then
  echo "dump_crash_log.sh [core filepath]"
else
#  echo "ok"
  rm -f /tmp/*-bt.txt
  /usr/local/CamShareServer/backtrace-from-core $1 /usr/local/freeswitch/bin/freeswitch
fi
