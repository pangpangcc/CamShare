-- common.lua
-- author : Max.Chiu
-- date : 2016/5/9
-- 公共函数

---- 将模块名设置为require的参数，重命名模块时，只需重命名文件名即可
--local modname = ...
---- 局部的变量
--local module = {}
---- 将这个局部变量最终赋值给模块名
--_G[modname] = module  

-- 字符串分隔
function string_split(str, sep)
    print("common::string_split(" .. 
    " str : '" .. str .. "'," ..
    " sep : '" .. sep .. "'" ..
    " )"
    );
    
    local tables = {};
    local i = 1;
    local j = 0;
    local value;
       
    while true do
        j = string.find(str, sep, i);
        if j == nil then
            value = string.sub(str, i, string.len(str));
            table.insert(tables, value);
            break;
        else
            value = string.sub(str, i, j - 1);
            table.insert(tables, value);
            
            i = j + string.len(sep);
        end;

    end
    return tables;
end

--return module;
