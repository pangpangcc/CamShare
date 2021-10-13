-- gen_dir_user_xml.lua
-- author : Max.Chiu
-- date : 2016/03/24
-- 用户登录脚本

local cjson = require "cjson"

dofile("/usr/local/freeswitch/scripts/common.lua");
dofile("/usr/local/freeswitch/scripts/site_config.lua");

freeswitch.consoleLog("NOTICE", "# 用户登录脚本->开始\n")
freeswitch.consoleLog("NOTICE", "# 用户登录脚本->params:\n" .. params:serialize() .. "\n")

api = freeswitch.API();

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
 -- H5媒体网关转发
local req_mediaserver = params:getHeader("mediaserver")

---- it's probably wise to sanitize input to avoid SQL injections !
--local my_query = string.format("select * from users where domain = '%s' and `%s`='%s' limit 1",
--  req_domain, req_key, req_user)

-- 用户信息字符串
XML_STRING = "";

local support_test = is_support_test();
local result = is_no_check();

-- 判断是否从H5网关转发
if req_mediaserver == "1" then
  result = 1
  freeswitch.consoleLog("DEBUG", "# 用户登录脚本->账号:" .. req_user .. "是H5媒体网关账号\n");
else
  -- 判断是否测试帐号
  if support_test == 1 then
    freeswitch.consoleLog("NOTICE", "# 用户登录脚本->账号:" .. req_user .. "检查是否测试账号\n");
    local index = string.find(req_user, "^WW%d+$");
    if index == nil then
      index = string.find(req_user, "^MM%d+$");
    end
    
    if index ~= nil then
      value = string.sub(req_user, index, string.len(req_user));
      local size = string.len(value);
  --    freeswitch.consoleLog("DEBUG", "# 用户登录脚本->账号:" .. req_user .. "测试账号, 检测长度 size : " .. size .. "\n");
      
      if( size < 7 ) then
        result = 1
        freeswitch.consoleLog("DEBUG", "# 用户登录脚本->账号:" .. req_user .. "测试账号, 检测长度通过\n");
      else 
        freeswitch.consoleLog("DEBUG", "# 用户登录脚本->账号:" .. req_user .. "测试账号, 检测长度失败\n");
      end
    else
      freeswitch.consoleLog("NOTICE", "# 用户登录脚本->账号:" .. req_user .. "不是测试账号\n");
    end
  end
end

-- 从接口验证用户
if result == 0 then 
  local loginPath = getLoginPath(siteId);
  if( loginPath ~= nil ) then
    local url = loginPath .. "&userId=" .. req_user .. " json connect-timeout 10 timeout 30 post " .. custom;
--    freeswitch.consoleLog("NOTICE", "# 用户登录脚本->发起http请求 " .. url .. "\n");
    response = api:execute("curl", url);
    if response ~= nil then
      freeswitch.consoleLog("NOTICE", "# 用户登录脚本->获取http返回:" .. url .. ", response:" .. response .. "\n");
      json = cjson.decode(response);
      body = json["body"];
      if body ~= nil then
        json = cjson.decode(body);
      
        result = json["result"];
        errno = json["errno"];
        errmsg = json["errmsg"];
      else
        freeswitch.consoleLog("WARNING", "# 用户登录脚本->http返回协议解析失败:" .. url .. "\n");
      end
    else
      freeswitch.consoleLog("WARNING", "# 用户登录脚本->获取http返回失败:" .. url .. "\n");
    end
  else
    freeswitch.consoleLog("WARNING", "# 用户登录脚本->没有找到http URL, siteId:" .. siteId .. ", req_user:" .. req_user .. "\n");
  end
end

-- 登录成功
if result == 1 then
  freeswitch.consoleLog("NOTICE", "# 用户登录脚本->账号:" .. req_user .. "登录成功\n");
	
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
else
  freeswitch.consoleLog("WARNING", "# 用户登录脚本->账号:" .. req_user .. "登录失败\n");
  XML_STRING = 
  [[<?xml version="1.0" encoding="UTF-8" standalone="no"?>
  <document type="freeswitch/xml">
  </document>]]
end

--freeswitch.consoleLog("NOTICE", "# 用户登录脚本->xml:\n" .. XML_STRING .. "\n")
freeswitch.consoleLog("NOTICE", "# 用户登录脚本->结束\n")