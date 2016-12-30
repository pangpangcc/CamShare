-- event_conference.lua
-- author : Max.Chiu
-- date : 2016/03/24
-- freeswitch会议事件监听脚本

api = freeswitch.API();

--freeswitch.consoleLog("NOTICE", "# 会议事件监听脚本->开始");
-- 获取事件类型
fun = event:getHeader("Event-Calling-Function");
--freeswitch.consoleLog("NOTICE", "# 会议事件监听脚本->事件:[" .. fun .. "]\n");
--freeswitch.consoleLog("NOTICE", "# 会议事件监听脚本->event:\n" .. event:serialize("json") .. "\n");
  
-- 处理会议增加成员
if fun == "conference_member_add" then
  conference_name = event:getHeader("Conference-Name");
  member_type = event:getHeader("Member-Type");
  member_id = event:getHeader("Member-ID");
  channel_id = event:getHeader("Channel-Call-UUID");
  freeswitch.consoleLog("NOTICE", "# 会议事件监听脚本->事件:channel_id:" .. channel_id .. "\n");
  
  -- 静音, 静视频
  -- 关别人看
  api:execute("conference", conference_name .. " mute " .. member_id);
  api:execute("conference", conference_name .. " vmute " .. member_id);
  -- 关自己看
  api:execute("conference", conference_name .. " relate " .. member_id .. " " .. member_id .. " clear");
  api:execute("conference", conference_name .. " relate " .. member_id .. " " .. member_id .. " sendvideo");
--  api:execute("conference", conference_name .. " relate " .. member_id .. " " .. member_id .. " nospeak");
--  api:execute("conference", conference_name .. " relate " .. member_id .. " " .. member_id .. " nohear");
  -- 关自己听
  api:execute("conference", conference_name .. " deaf " .. member_id);
  
  -- 根据用户类型设置权限
  if member_type == "moderator" then
    freeswitch.consoleLog("NOTICE", "# 会议事件监听脚本->事件:增加主持人[member_id:" .. member_id .. "]到会议室[" .. conference_name .. "]\n");   
    api:execute("conference", conference_name .. " vid-floor " .. member_id .. " force");
--    api:execute("uuid_setvar", channel_id .. " enable_file_write_buffering false");
--    api:execute("conference", conference_name .. " record /usr/local/freeswitch/recordings/video_h264/" .. conference_name .. os.date("_%Y%m%d%H%M%S", os.time()) .. ".h264");
  else
    freeswitch.consoleLog("NOTICE", "# 会议事件监听脚本->事件:增加成员[member_id:" .. member_id .. "]到会议室[" .. conference_name .. "]\n");
  end
  
end -- 处理会议增加成员完成

--freeswitch.consoleLog("NOTICE", "# 会议事件监听脚本->结束\n");
