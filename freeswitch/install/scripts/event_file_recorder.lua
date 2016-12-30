-- event_file_recorder.lua
-- author : Max.Chiu
-- date : 2016/03/24
-- freeswitch录制模块事件监听脚本

dofile("/usr/local/freeswitch/scripts/common.lua");

--freeswitch.consoleLog("NOTICE", "# 录制模块事件监听脚本->开始");

api = freeswitch.API();

local_domain_name = freeswitch.getGlobalVariable("domain_name")
freeswitcNOTICEleLog("local_domain_name", local_domain_name)

-- 获取事件类型
fun = event:getHeader("Event-Calling-Function");
--freeswitch.consoleLog("NOTICE", "# 录制模块事件监听脚本->事件:[" .. fun .. "]\n");
--freeswitch.consoleLog("NOTICE", "# 录制模块事件监听脚本->event:\n" .. event:serialize("json") .. "\n");
  
---- 处理会议增加成员
if fun == "file_record_send_command" then
  cmd = event:getHeader("cmd");
  freeswitch.consoleLog("NOTICE", "# 录制模块事件监听脚本->执行命令: " .. cmd .. " \n");
  
--  --  发起http请求, 获取拨号计划
--  response = api:execute("curl", "http://".. local_domain_name .. ":9200" .. 
--  "/EXECCMD?" .. 
--  "cmd=" .. cmd .. 
--  " json connect-timeout 5 timeout 10 get");
--  
----  解析返回
--  if response ~= nil then
--    freeswitch.consoleLog("NOTICE", "# 录制模块事件监听脚本->执行命令, 获取http返回:\n" .. response);
--  end
  
end -- 处理会议增加成员完成

freeswitch.consoleLog("NOTICE", "# 录制模块事件监听脚本->结束\n");
