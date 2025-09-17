#ifndef LUASCRIPTS_H
#define LUASCRIPTS_H

#include <QString>

namespace Lua {
    QString Env = R"(local function outputToConsole(...)
    local console <close> = io.open('Executor/console.txt', 'a+')
    if console then
        console:setvbuf('no')
        local args = {...}
        for _, line in pairs (args) do
            console:write(string.format('%s\n', tostring(line)))
        end
        console:flush()
    end
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
)";

QString GetMenuCode = QString(R"(local value = gui.GetValue("%1")
local file <close> = io.open("Executor/returnvalue.txt", "w")
if file then
    file:write(value)
    file:flush()
    file:close()
end
)");
};

#endif // LUASCRIPTS_H
