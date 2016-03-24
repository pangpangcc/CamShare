-- dialplan_internal_default.lua
-- author : Max.Chiu
-- date : 2016/3/24
-- 内网拨号计划

local cjson = require "cjson"

freeswitch.consoleLog("CONSOLE", "# 内网拨号计划->开始")

function logChannelVar(k, v)
  if not v then v = "nil" end
  freeswitch.consoleLog("CONSOLE", "# [" .. k .. " : " .. v .. "]")
end

-- 获取会话变量
uuid = session:getVariable("uuid")
rtmp_session = session:getVariable("rtmp_session")
destination_number = session:getVariable("destination_number")
domain_name = session:getVariable("domain_name")
network_addr = session:getVariable("network_addr")
local_domain_name = freeswitch.getGlobalVariable("domain_name")

logChannelVar("uuid", uuid)
logChannelVar("rtmp_session", rtmp_session)
logChannelVar("destination_number", destination_number)
logChannelVar("domain_name", domain_name)
logChannelVar("network_addr", network_addr)
logChannelVar("local_domain_name", local_domain_name)

-- 创建拨号规则
api = freeswitch.API();
response = api:execute("curl", "http://192.168.88.143:9876/GETUSERBYSESSION?session=" .. rtmp_session .. " json connect-timeout 5 timeout 10 get");
if response ~= nil then
  freeswitch.consoleLog("CONSOLE", "# 内网拨号计划->获取http返回:\n" .. response);
  json = cjson.decode(response);
  body = json["body"];
  if body ~= nil then
    json = cjson.decode(body);
    caller = json["caller"];
    
    -- 会议
    ACTIONS = {
      {"log", "CONSOLE # 内网拨号计划->Action开始"},
      {"set", "hangup_after_bridge=true"},
      {"set", "continue_on_fail=false"},
      {"set", "bypass_media_after_bridge=false"}
    } 
    
    if( destination_number == caller ) then
      table.insert(ACTIONS, {"conference", destination_number .. "@default++flags{moderator}"})
      freeswitch.consoleLog("CONSOLE", "# 内网拨号计划->" .. destination_number .. "@default++flags{moderator}");
    else
      table.insert(ACTIONS, {"conference", destination_number .. "@default"})
      freeswitch.consoleLog("CONSOLE", "# 内网拨号计划->" .. destination_number .. "@default");
    end
  else 
     freeswitch.consoleLog("CONSOLE", "# 内网拨号计划->json解析失败");
  end
else
  freeswitch.consoleLog("CONSOLE", "# 内网拨号计划->获取http返回失败");
end

freeswitch.consoleLog("CONSOLE", "# 内网拨号计划->结束")