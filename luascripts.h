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
};

#endif // LUASCRIPTS_H
