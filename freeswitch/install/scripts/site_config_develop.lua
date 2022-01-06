-- site_config.lua
-- author : Max.Chiu
-- date : 2016/5/9
-- 公共函数

---- 将模块名设置为require的参数，重命名模块时，只需重命名文件名即可
--local modname = ...
---- 局部的变量
--local module = {}
---- 将这个局部变量最终赋值给模块名
--_G[modname] = module  

-- php接口路径
loginPaths = {
  ["0"] = "http://192.168.88.136/livechat/setstatus.php?action=getuserloginstatus",
  ["1"] = "http://192.168.88.136/livechat/setstatus.php?action=getuserloginstatus",
  ["4"] = "http://192.168.88.136/livechat/setstatus.php?action=getuserloginstatus",
  ["5"] = "http://192.168.88.136/livechat/setstatus.php?action=getuserloginstatus",
  ["6"] = "http://192.168.88.136/livechat/setstatus.php?action=getuserloginstatus",
  ["7"] = "https://192.168.88.136/chat/setStatus?action=getuserloginstatus"
};

function getLoginPath(siteId)
  loginPath = nil;
  if loginPaths[siteId] ~= nil then
    loginPath = loginPaths[siteId];
  end
  return loginPath;
end

-- 是否支持测试帐号
function is_support_test()
    return 1;
end

-- 是否需要验证
function is_no_check()
  return 1;
end
--return module;
