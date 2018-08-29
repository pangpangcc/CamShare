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
  ["0"] = "http://www.chnlove.com/livechat/setstatus.php?action=getuserloginstatus",
  ["1"] = "http://www.idateasia.com/livechat/setstatus.php?action=getuserloginstatus",
  ["4"] = "http://www.charmdate.com/livechat/setstatus.php?action=getuserloginstatus",
  ["5"] = "http://www.latamdate.com/livechat/setstatus.php?action=getuserloginstatus",
  ["6"] = "http://www.asiame.com/livechat/setstatus.php?action=getuserloginstatus"
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
    return 0;
end

--return module;
