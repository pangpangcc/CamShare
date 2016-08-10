#!/bin/sh

# --- stop camshare & freeswitch ---
# stop crontab
systemctl stop crond
# stop camshare & freeswitch
/usr/local/CamShareServer/stop.sh

# ---- freeswitch ----
# freeswitch ./lib & ./mod files
cp -f ./file/lib/* /usr/local/freeswitch/lib/
cp -f ./file/mod/* usr/local/freeswitch/mod/

# freeswitch scripts
#cp -f ./file/*.lua /usr/local/freeswitch/scripts/

# freeswitch mod_file_recorder shell
cp -f ./file/close_shell.sh /usr/local/freeswitch/bin/mod_file_recorder_sh/

# freeswitch mod_file_recorder
#cp -f ./file/mod_file_recorder* /usr/local/freeswitch/mod/

# freeswitch mod_rtmp
#cp -f ./file/mod_rtmp* /usr/local/freeswitch/mod/

# ---- camshare ----
# camshare-middleware file
cp -f ./file/camshare-middleware /usr/local/CamShareServer/
#cp -f ./file/camshare-middleware.config /usr/local/CamShareServer/

# camshare shell
#cp -f ./file/run.sh /usr/local/CamShareServer/
#cp -f ./file/stop.sh /usr/local/CamShareServer/
#cp -f ./file/check_run.sh /usr/local/CamShareServer/

# --- start camshare & freeswitch ---
# run camshare & freeswitch
/usr/local/CamShareServer/run.sh
# start crontab
systemctl start crond
