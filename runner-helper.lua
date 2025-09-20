--- made by navet
--- helper script for Lua Runner

local json = load(http.Get("https://dkolf.de/dkjson-lua/dkjson-2.8.lua"))()
assert(type(json) == "table", "JSON is not a table!")

local function textencode(str)
    return (str:gsub("([^%w%-_%.~])", function(c)
        return string.format("%%%02X", string.byte(c))
    end))
end

local function SetRealtimeText(str)
    local encoded = textencode(str)
    http.Get(string.format("http://localhost:8080/setrealtime=%s", encoded))
end

local function customPrint(...)
    local text = ""
    local args = {...}
    local len = #args

    for i = 1, len do
        local str = tostring(args[i])
        if (i > 1) then -- not the first element?
            str = "\t".. str -- add a "tab" before it
        end

        text = text .. str

        --- this part is not on the official Lua source code
        --- but i dont want gaps in the console
        if (len ~= i) then -- last element?
            text = text .. "\n"
        end
    end

    http.Get(string.format("http://localhost:8080/appendconsole=%s", textencode(text)))
end

local function customPrintColored(r, g, b, a, ...)
    local text = ""
    local args = {...}
    local len = #args

    for i = 1, len do
        local str = tostring(args[i])
        if (i > 1) then -- not the first element?
            str = "\t".. str -- add a "tab" before it
        end

        text = text .. str

        --- this part is not on the official Lua source code
        --- but i dont want gaps in the console
        if (len ~= i) then -- last element?
            text = text .. "\n"
        end
    end

    if (r > 255) then
        r = 255
    elseif (r < 0) then
        r = 0
    end

    if (g > 255) then
        g = 255
    elseif (g < 0) then
        g = 0
    end

    if (b > 255) then
        b = 255
    elseif (b < 0) then
        b = 0
    end

    r = tostring(r)
    g = tostring(g)
    b = tostring(b)
    a = tostring(a)

    if (#r < 3) then
        local charcount = #r
        local amount_to_3 = 3 - charcount
        while (amount_to_3 ~= 0) do
            r = "0" .. r
            amount_to_3 = amount_to_3 - 1
        end
    end

    if (#g < 3) then
        local charcount = #g
        local amount_to_3 = 3 - charcount
        while (amount_to_3 ~= 0) do
            g = "0" .. g
            amount_to_3 = amount_to_3 - 1
        end
    end

    if (#b < 3) then
        local charcount = #b
        local amount_to_3 = 3 - charcount
        while (amount_to_3 ~= 0) do
            b = "0" .. b
            amount_to_3 = amount_to_3 - 1
        end
    end

    if (#a < 3) then
        local charcount = #a
        local amount_to_3 = 3 - charcount
        while (amount_to_3 ~= 0) do
            a = "0" .. a
            amount_to_3 = amount_to_3 - 1
        end
    end

    http.Get(string.format("http://localhost:8080/appendconsolecolor=%s%s%s%s%s", r, g, b, a, textencode(text)))
end

local function customWarn(...)
    local args = {...}
    local text = ""

    for i = 1, #args do
        text = text .. "[Warn] " .. tostring(args[i])
        if (#args ~= i) then -- last element?
            text = text .. "\n"
        end
        http.Get(string.format("http://localhost:8080/appendconsole=%s", textencode(text)))
    end
end

---@param message string
---@param level integer?
local function customError(message, level)
    level = level or 0

    if (level > 0) then
        local info = debug.getinfo(2)
        --- lua src code uses lua_where but currentline should work fine
        message = string.format("Line: %i | %s", info.currentline, message)
    end

    http.Get(string.format("http://localhost:8080/appendconsole=%s", textencode("[Error] " .. message)))
    return message
end

local function SendCallbackList(list)
    local encoded = json.encode(list)
    assert(encoded or type(encoded) ~= "string", "Encoded callback list is wrong!")
    http.Get(string.format("http://localhost:8080/setcallbacklist=%s", encoded))
end

local env = {}
setmetatable(env, {__index = _ENV})

env.textencode = textencode
env.SetRealtimeText = SetRealtimeText
env.print = customPrint
env.error = customError
env.warn = customWarn
env.printc = customPrintColored

env.__sendcallbacklist = SendCallbackList
env.__origRegister = callbacks.Register
env.__origUnregister = callbacks.Unregister

env.__callbackList = {}
env.__callbackCounter = 0
env.callbacks = {}
env.callbacks.Register = function(...)
    local args = {...}
    local size = #args
    local callback, func, name

    if (size == 2) then
        if (args[1] == nil) then error("Argument #1 is nil!") return false end
        if (args[2] == nil) then error("Argument #2 is nil!") return false end

        callback = tostring(args[1])

        if (#callback == 0) then
            error("Callback type is empty!")
            return false
        end

        func = args[2]
        if (type(func) ~= "function") then error("Callback function is not a function!") return false end

        env.__callbackCounter = env.__callbackCounter + 1
        name = string.format("unnamed-%i", env.__callbackCounter)

        if (env.__origRegister(callback, func)) then
            table.insert(env.__callbackList, {["callback"] = callback, ["name"] = name})
            env.__sendcallbacklist(env.__callbackList)
            return true
        end

    elseif (size == 3) then
        if (args[1] == nil) then error("Argument #1 is nil!") return false end
        if (args[2] == nil) then error("Argument #2 is nil!") return false end
        if (args[3] == nil) then error("Argument #3 is nil!") return false end

        callback = tostring(args[1])
        if (#callback == 0) then
            error("Callback type is empty!")
            return false
        end

        name = tostring(args[2])
        if (#name == 0) then
            error("Callback name is empty!")
            return false
        end

        func = args[3]
        if (type(func) ~= "function") then
            error("Callback function is not a function!")
            return false
        end

        if (env.__origRegister(callback, name, func)) then
            table.insert(env.__callbackList, {["callback"] = callback, ["name"] = name})
            env.__sendcallbacklist(env.__callbackList)
        end
    else
        error("Invalid number of arguments!")
        return false
    end

    return true
end

env.callbacks.Unregister = function(callback, name)
    for i, call in ipairs(env.__callbackList) do
        if (call["callback"] == callback and call["name"] == name) then
            if (env.__origUnregister(callback, name)) then
                table.remove(env.__callbackList, i)
                env.__sendcallbacklist(env.__callbackList)
                return true
            end
        end
    end

    return false
end

local function poll()
    local script = http.Get("http://localhost:8080/getcurrentscript")
    if (script ~= "") then
        local func, err = load(script, nil, "t", env)
        if (func) then
            pcall(func)
        else
            error(tostring(err))
        end
    end
end

callbacks.Unregister("Draw", "LuaRunnerDraw")
callbacks.Register("Draw", "LuaRunnerDraw", poll)
callbacks.Register("Unload", function (...)
    for i, callback in ipairs(env.__callbackList) do
        callbacks.Unregister(callback["callback"], callback["name"])
    end

    env.__callbackList = {}
    env.__sendcallbacklist(env.__callbackList)
end)