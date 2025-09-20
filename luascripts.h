#ifndef LUASCRIPTS_H
#define LUASCRIPTS_H

#include <QString>

namespace Lua {
QString GetMenuCode = QString(R"(local value = gui.GetValue("%1")
local file <close> = io.open("Executor/returnvalue.txt", "w")
if file then
    file:write(value)
    file:flush()
    file:close()
end
)");

QString Env = QString(R"(local function textencode(str)
    return (str:gsub('([^%w%-_%.~])'))
end

local function outputToConsole(...)
    local text = table.concat({...}, "\n")
    local encoded = textencode(text)
    http.Get(string.format("http://localhost:8080/appendconsole=%s", encoded))
end

local function print(...)
    local args = {...}

    for i = 1, #args do
        if (args[i]) then
            outputToConsole(string.format('[Print] %s', tostring(args[i])))
        end
    end
end

local function error(...)
    local args = {...}
    for i = 1, #args do
        if (args[i]) then
            outputToConsole(string.format('[Error] %s', tostring(args[i])))
        end
    end
end

local function warn(...)
    local args = {...}
    for i = 1, #args do
        if (args[i]) then
            outputToConsole(string.format('[Warn] %s', tostring(args[i])))
        end
    end
end

local function SetRealtimeText(str)
    local encoded = textencode(str)
    http.Get(string.format("http://localhost:8080/setrealtime=%s", encoded))
end
)");
};

#endif // LUASCRIPTS_H
