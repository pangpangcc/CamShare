-- gen_dir_user_xml.lua
-- author : Max.Chiu
-- date : 2016/03/24
-- 用户登陆脚本

local cjson = require "cjson"

freeswitch.consoleLog("CONSOLE", "# 用户登陆脚本->开始\n")
freeswitch.consoleLog("CONSOLE", "# 用户登陆脚本->params:\n" .. params:serialize() .. "\n")

api = freeswitch.API();

-- 字符串分隔
function string_split(str, sep)
    local tables = {};
    local i = 0;
    local j = 0;
    local value;
    
    while true do
        j = string.find(str, sep, i + 1);
        if j == nil then
            value = string.sub(str, i + 1, string.len(str));
            table.insert(tables, value);
            break;
        else
            print("i : " .. i .. ", j : " ..  j)
            value = string.sub(str, i + 1, j - 1);
            table.insert(tables, value);
        end;
        
        i = j;
    end
    return tables;
end

local req_domain = params:getHeader("domain")
local req_key    = params:getHeader("key")
-- 用户名
local req_user   = params:getHeader("user")
-- 认证密码
local req_pass   = "123456"
-- 站点
local req_site   = params:getHeader("site")
local siteId     = ""
if req_site ~= nil then 
  siteId = req_site
end
-- 客户端自定义参数
local req_custom = params:getHeader("custom")
local custom = ""
if req_custom ~= nil then
  custom = req_custom
end
 
---- it's probably wise to sanitize input to avoid SQL injections !
--local my_query = string.format("select * from users where domain = '%s' and `%s`='%s' limit 1",
--  req_domain, req_key, req_user)

-- 用户信息字符串
XML_STRING = "";

-- php接口路径
loginPaths = {
  ["0"] = "http://test:5179@demo.chnlove.com/livechat/setstatus.php?action=getuserloginstatus",
  ["1"] = "http://test:5179@demo.idateasia.com/livechat/setstatus.php?action=getuserloginstatus",
  ["4"] = "http://test:5179@demo.latamdate.com/livechat/setstatus.php?action=getuserloginstatus",
  ["5"] = "http://test:5179@demo.latamdate.com/livechat/setstatus.php?action=getuserloginstatus",
};

XML_STRING = ""
  
-- 从接口验证用户
loginPath = loginPaths["0"];
if loginPaths[siteId] ~= nil then
  loginPath = loginPaths[siteId]
end

freeswitch.consoleLog("CONSOLE", "# 用户登陆脚本->发起http请求 " .. loginPath .. "\n");
response = api:execute("curl", loginPath .. "&userId=" .. req_user .. " json connect-timeout 10 timeout 30 post " .. custom);
if response ~= nil then
  freeswitch.consoleLog("CONSOLE", "# 用户登陆脚本->获取http返回:\n" .. response .. "\n");
  json = cjson.decode(response);
  body = json["body"];
  if body ~= nil then
    json = cjson.decode(body);
    
    result = json["result"];
    errno = json["errno"];
    errmsg = json["errmsg"];
    
    if result == 1 then
--    登陆成功
      XML_STRING = 
      [[<?xml version="1.0" encoding="UTF-8" standalone="no"?>
      <document type="freeswitch/xml">
        <section name="directory">
          <domain name="]] .. req_domain .. [[">
            <user id="]] .. req_user .. [[" effective_caller_id_number="]] .. req_user .. [[">
              <params>
                <param name="password" value="]] .. "" .. [["/>
                <param name="allow-empty-password" value="true"/>
                <param name="site-id" value="]] .. siteId .. [["/>
              </params>
              <variables>
              </variables>
            </user>
          </domain>
        </section>
      </document>]]
    end
  end
else
  freeswitch.consoleLog("CONSOLE", "# 用户登陆脚本->获取http返回失败");
end

freeswitch.consoleLog("CONSOLE", "# 用户登陆脚本->xml:\n" .. XML_STRING .. "\n")
freeswitch.consoleLog("CONSOLE", "# 用户登陆脚本->结束\n")