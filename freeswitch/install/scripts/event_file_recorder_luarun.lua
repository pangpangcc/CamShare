-- event_file_recorder.lua
-- author : Max.Chiu
-- date : 2016/03/24
-- freeswitch录制模块事件监听脚本

dofile("/usr/local/freeswitch/scripts/common.lua");

api = freeswitch.API();
--con = freeswitch.EventConsumer("all");
con = freeswitch.EventConsumer("CUSTOM","file_recorder::maintenance",30000); 
for event in (function() return con:pop(1) end) do 

--freeswitch.consoleLog("NOTICE", "# 录制模块事件监听脚本->开始");

local_domain_name = freeswitch.getGlobalVariable("domain_name")
freeswitch.consoleLog("local_domain_name", local_domain_name)

-- 获取事件类型
fun = event:getHeader("Event-Calling-Function");
--freeswitch.consoleLog("DEBUG", "# 录制模块事件监听脚本->事件:[" .. fun .. "]\n");
--freeswitch.consoleLog("DEBUG", "# 录制模块事件监听脚本->event:\n" .. event:serialize("json") .. "\n");
  
---- 处理会议增加成员
if fun == "file_record_send_command" then
  cmd = event:getHeader("cmd");
  --  发起http请求, 通知执行命令
  local encode_url = "/EXEC?" .. "CMD=" .. cmd
  encode_url = api:execute("url_encode", encode_url);
  local url = "http://" .. local_domain_name .. ":9201" .. encode_url .. " json connect-timeout 5 timeout 10 get";
  freeswitch.consoleLog("DEBUG", "# 录制模块事件监听脚本(luarun)->发起http请求:" .. url .. "\n");
  response = api:execute("curl", url);
  
  --  解析返回
  if response ~= nil then
    freeswitch.consoleLog("DEBUG", "# 录制模块事件监听脚本(luarun)->获取http返回:" .. response .. "\n");
  else
    freeswitch:consoleLog("WARNING", "# 录制模块事件监听脚本(luarun)->获取http返回失败:" .. url .."\n");
  end
  
end -- 处理会议增加成员完成

--freeswitch.consoleLog("DEBUG", "# 录制模块事件监听脚本->结束\n");
end