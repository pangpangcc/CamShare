-- event_luarun_sample.lua
-- author : Max.Chiu
-- date : 2016/03/24

dofile("/usr/local/freeswitch/scripts/common.lua");

api = freeswitch.API();

event_name = "file_recorder::maintenance";

--con = freeswitch.EventConsumer("all");
con = freeswitch.EventConsumer("CUSTOM", event_name, 30000); 
for event in (function() return con:pop(1) end) do 

--freeswitch.consoleLog("NOTICE", "# luarun事件监听脚本->开始");

local_domain_name = freeswitch.getGlobalVariable("domain_name")
freeswitch.consoleLog("local_domain_name", local_domain_name)

-- 获取事件类型
fun = event:getHeader("Event-Calling-Function");
--freeswitch.consoleLog("DEBUG", "# luarun事件监听脚本->事件:[" .. fun .. "]\n");
--freeswitch.consoleLog("DEBUG", "# luarun事件监听脚本->event:\n" .. event:serialize("json") .. "\n");

event_fuction_name = "file_record_send_command";
  
if fun == event_fuction_name then
  cmd = event:getHeader("cmd");
end

--freeswitch.consoleLog("DEBUG", "# luarun事件监听脚本->结束\n");
end