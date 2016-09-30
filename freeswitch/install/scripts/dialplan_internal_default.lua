-- dialplan_internal_default.lua
-- author : Max.Chiu
-- date : 2016/3/24
-- 内网拨号计划

local cjson = require "cjson"

dofile("/usr/local/freeswitch/scripts/common.lua");

freeswitch.consoleLog("CONSOLE", "# 内网拨号计划->开始\n")

-- 创建拨号规则
api = freeswitch.API();

-- 输出变量
function logChannelVar(k, v)
  if not v then v = "nil" end
  session:consoleLog("CONSOLE", "# [" .. k .. " : " .. v .. "]\n")
end

-- 获取会话变量
source = session:getVariable("source")
uuid = session:getVariable("uuid")
caller = session:getVariable("caller")
destination_number = session:getVariable("destination_number")
domain_name = session:getVariable("domain_name")
network_addr = session:getVariable("network_addr")
local_domain_name = freeswitch.getGlobalVariable("domain_name")

logChannelVar("source", source)
logChannelVar("uuid", uuid)
logChannelVar("caller", caller)
logChannelVar("destination_number", destination_number)
logChannelVar("domain_name", domain_name)
logChannelVar("network_addr", network_addr)
logChannelVar("local_domain_name", local_domain_name)

-- 会议室Id
local conference = "";
-- 服务器Id
local serverId = "";
-- 站点Id
local siteId = "";

-- 分隔[用户Id]/[Livechat服务器Id]/[站点Id]
tables = string_split(destination_number, "|||");
if( #tables >= 3 ) then
  conference = tables[1];
  serverId = tables[2];
  siteId = tables[3];
  
--  设置channel变量
  session:setVariable("serverId", serverId);
  session:setVariable("siteId", siteId);
    
--  发起http请求, 获取拨号计划
  response = api:execute("curl", "http://".. local_domain_name .. ":9200" .. 
  "/GETDIALPLAN?" .. 
  "caller=" .. caller .. 
  "&channelId=" .. uuid ..
  "&conference=" .. conference .. 
  "&serverId=" .. serverId .. 
  "&siteId=" .. siteId .. 
  " json connect-timeout 5 timeout 10 get");
  
--  解析返回
  if response ~= nil then
    session:consoleLog("CONSOLE", "# 内网拨号计划->获取http返回:\n" .. response);
    json = cjson.decode(response);
    body = json["body"];
    if body ~= nil then
      json = cjson.decode(body);
      ret = json["ret"];
      
      if ret == 1 then
        -- 会议
        ACTIONS = {
            {"set", "hangup_after_bridge=true"},
            {"set", "continue_on_fail=false"},
            {"set", "bypass_media_after_bridge=false"},
        }
      
        if( conference == caller ) then
--          进入自己会议室
--          table.insert(ACTIONS, {"set", "enable_file_write_buffering=false"})
--          table.insert(ACTIONS, {"record_fsv", "$${base_dir}/recordings/" .. destination_number .. "-${strftime(%Y-%m-%d-%H-%M-%S)}.fsv"})
            table.insert(ACTIONS, {"conference", conference .. "@default++flags{moderator}"})
            session:consoleLog("CONSOLE", "# 内网拨号计划->" .. conference .. "@default++flags{moderator}\n");
        else
--          进入别人会议室
            table.insert(ACTIONS, {"conference", conference .. "@default"})
            session:consoleLog("CONSOLE", "# 内网拨号计划->" .. conference .. "@default\n");
        end
      else
          session:consoleLog("CONSOLE", "# 内网拨号计划->caller解析失败\n");
      end

    else
      session:consoleLog("CONSOLE", "# 内网拨号计划->json解析失败\n");
    end
    
  else
    session:consoleLog("CONSOLE", "# 内网拨号计划->获取http返回失败\n");
  end
else
    session:consoleLog("CONSOLE", "# 内网拨号计划->解析[用户Id]/[Livechat服务器Id]/[站点Id]错误失败\n");
end

session:consoleLog("CONSOLE", "# 内网拨号计划->结束\n")