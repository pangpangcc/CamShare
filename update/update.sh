#!/bin/sh

# mod_file_recorder shell
cp -f ./file/close_shell.sh /usr/local/freeswitch/bin/mod_file_recorder_sh/

# scripts
#cp -f ./file/event_conference.lua /usr/local/freeswitch/scripts/
#cp -f ./file/dialplan_internal_default.lua /usr/local/freeswitch/scripts/
#cp -f ./file/gen_dir_user_xml.lua /usr/local/freeswitch/scripts/

# mod_file_recorder
cp -f ./file/mod_file_recorder* /usr/local/freeswitch/mod/

# camshare
cp -f ./file/camshare-middleware /usr/local/CamShareServer/
cp -f ./file/camshare-middleware.config /usr/local/CamShareServer/
