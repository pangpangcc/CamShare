-- gen_dir_user_xml.lua
-- author : Max.Chiu
-- date : 2016/03/24
-- 用户登陆脚本

local cjson = require "cjson"

api = freeswitch.API();

freeswitch.consoleLog("CONSOLE", "# 用户登陆脚本->开始")
freeswitch.consoleLog("CONSOLE", "# 用户登陆脚本->params:\n" .. params:serialize())

local req_domain = params:getHeader("domain")
local req_key    = params:getHeader("key")
local req_user   = params:getHeader("user")
 
assert (req_domain and req_key and req_user,
  "# This example script only supports generating directory xml for a single user \n")
 
local dbh = freeswitch.Dbh("odbc://freeswitch:freeswitch:123456")
if dbh:connected() == false then
  freeswitch.consoleLog("CONSOLE", "# 用户登陆脚本->连接数据库失败")
  if dsn ~= nil then 
    freeswitch.consoleLog("CONSOLE", "# 用户登陆脚本->连接数据库失败 dsn : " .. dsn)
  end
  return
end
 
-- it's probably wise to sanitize input to avoid SQL injections !
local my_query = string.format("select * from users where domain = '%s' and `%s`='%s' limit 1",
  req_domain, req_key, req_user)
 
XML_STRING = ""; 

-- 从数据库验证
assert (dbh:query(my_query, function(u) -- there will be only 0 or 1 iteration (limit 1)
XML_STRING = 
[[<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<document type="freeswitch/xml">
  <section name="directory">
    <domain name="]] .. u.domain .. [[">
      <user id="]] .. u.id .. [[" effective_caller_id_number="]] .. u.id .. [[">
        <params>
          <param name="password" value="]].. u.password .. [["/>
        </params>
        <variables>
        </variables>
      </user>
    </domain>
  </section>
</document>]]
end))

--XML_STRING = "";  
-- 从接口验证用户
--freeswitch.consoleLog("CONSOLE", "# 用户登陆脚本->发起http请求 http://192.168.88.143");
--response = api:execute("curl", "http://192.168.88.143 json connect-timeout 10 timeout 30 get");
--if response ~= nil then
--  freeswitch.consoleLog("CONSOLE", "# 用户登陆脚本->获取http返回:\n" .. response);
--  json = cjson.decode(response);
--  body = json["body"];
--  if body ~= nil then
--    json = cjson.decode(body);
--    username = json["username"];
--    password = json["password"];
--    domain = json["domain"];
--  end
--else
--  freeswitch.consoleLog("CONSOLE", "# 用户登陆脚本->获取http返回失败");
--end

freeswitch.consoleLog("CONSOLE", "# 用户登陆脚本->xml:\n" .. XML_STRING)
freeswitch.consoleLog("CONSOLE", "# 用户登陆脚本->结束")