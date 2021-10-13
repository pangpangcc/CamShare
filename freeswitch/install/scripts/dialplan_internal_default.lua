-- dialplan_internal_default.lua
-- author : Max.Chiu
-- date : 2016/3/24
-- 内网拨号计划

local cjson = require "cjson"

dofile("/usr/local/freeswitch/scripts/common.lua");

freeswitch.consoleLog("NOTICE", "# 内网拨号计划->开始\n")

-- 创建拨号规则
api = freeswitch.API();

-- 输出变量
function logChannelVar(k, v)
  if not v then v = "nil" end
  session:consoleLog("NOTICE", "# 内网拨号计划->[" .. k .. " : " .. v .. "]\n")
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
-- 会话类型
local chat_type_string = ""

-- 分隔[用户Id]/[Livechat服务器Id]/[站点Id]
tables = string_split(destination_number, "|||");
if( #tables >= 3 ) then
  conference = tables[1];
  serverId = tables[2];
  siteId = tables[3];
  
--  Add by Max 2020/07/15
  if( #tables >= 4 ) then
    chat_type_string = tables[4];
    session:setVariable("chat_type_string", chat_type_string);
    logChannelVar("chat_type_string", chat_type_string)
  end
  
--  设置channel变量
  session:setVariable("serverId", serverId);
  session:setVariable("siteId", siteId);
    
--  发起http请求, 获取拨号计划
  local url = "http://".. local_domain_name .. ":9200" .. 
                      "/GETDIALPLAN?" .. 
                      "caller=" .. caller .. 
                      "&channelId=" .. uuid ..
                      "&conference=" .. conference .. 
                      "&serverId=" .. serverId .. 
                      "&siteId=" .. siteId .. 
                      "&source=" .. source .. 
                      "&chat_type_string=" .. chat_type_string .. 
                      " json connect-timeout 5 timeout 10 get";
--  freeswitch.consoleLog("NOTICE", "# 内网拨号计划->发起http请求 " .. url .. "\n");
  response = api:execute("curl", url);
  
--  解析返回
  if response ~= nil then
    session:consoleLog("NOTICE", "# 内网拨号计划->获取http返回:" .. url .. ", response:" .. response .. "\n");
    json = cjson.decode(response);
    body = json["body"];
    if body ~= nil then
      json = cjson.decode(body);
      ret = json["ret"];
      flags = "@default++flags{mute|vmute|deaf}";
      
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
            flags = "@default++flags{moderator|mute|vmute|deaf}";
        else
--          进入别人会议室
            flags = "@default++flags{mute|vmute|deaf}";
        end
        table.insert(ACTIONS, {"conference", conference .. flags})
        session:consoleLog("NOTICE", "# 内网拨号计划->" .. conference .. flags .. "\n");
        
      else
          session:consoleLog("NOTICE", "# 内网拨号计划->caller解析失败:" .. url .. "\n");
      end

    else
      session:consoleLog("NOTICE", "# 内网拨号计划->json解析失败:" .. url .. "\n");
    end
    
  else
    session:consoleLog("NOTICE", "# 内网拨号计划->获取http返回失败:" .. url .. "\n");
  end
else
    session:consoleLog("NOTICE", "# 内网拨号计划->解析[用户Id]/[Livechat服务器Id]/[站点Id]错误失败" .. url .. "\n");
end

session:consoleLog("NOTICE", "# 内网拨号计划->结束\n")