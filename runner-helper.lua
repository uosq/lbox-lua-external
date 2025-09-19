--- made by navet
--- helper script for Lua Runner

local json = load(http.Get("https://raw.githubusercontent.com/LuaDist/dkjson/refs/heads/master/dkjson.lua"))()
assert(json, "JSON is nil!")

local callbackCounter = 0

local function textencode(str)
    return (str:gsub('([^%w%-_%.~])', function(c)
        return string.format('%%%02X', string.byte(c))
    end))
end

local env = _ENV

env.outputToConsole = function(...)
    local text = table.concat({...}, "\n")
    local encoded = textencode(text)
    http.Get(string.format("http://localhost:8080/appendconsole=%s", encoded))
end

env.print = function(...)
    local args = {...}

    for i = 1, #args do
        if (args[i]) then
            outputToConsole(string.format('[Print] %s', tostring(args[i])))
        end
    end
end

env.error = function(...)
    local args = {...}
    for i = 1, #args do
        if (args[i]) then
            outputToConsole(string.format('[Error] %s', tostring(args[i])))
        end
    end
end

env.warn = function(...)
    local args = {...}
    for i = 1, #args do
        if (args[i]) then
            outputToConsole(string.format('[Warn] %s', tostring(args[i])))
        end
    end
end

env.SetRealtimeText = function(str)
    local encoded = textencode(str)
    http.Get(string.format("http://localhost:8080/setrealtime=%s", encoded))
end

local origRegister = callbacks.Register
local origUnregister = callbacks.Unregister

local callbackList = {}

local function sendCallbackList()
    local list = {}
    for _, call in ipairs(callbackList) do
        table.insert(list, {callback = call[1], name = call[2]})
    end
    http.Get(string.format("http://localhost:8080/setcallbacklist=%s", textencode(json.encode(list))))
end

env.callbacks = {
    Register = function(...)
        local args = {...}
        local callback = tostring(args[1])
        if (not callback or callback == "nil") then
            error("Invalid callback name!")
            return false
        end

        if (#args == 2) then
            local func = args[2]
            if (type(func) == "function") then
                callbackCounter = callbackCounter + 1
                local newName = tostring(callbackCounter)

                if (origRegister(callback, newName, func)) then
                    table.insert(callbackList, {callback, newName})
                    sendCallbackList()
                    return true
                end
            else
                error("Argument #2 is not a function!")
            end

        elseif (#args == 3) then
            local name, func

            name = args[2]
            func = args[3]

            if (name and func) then
                if (type(name) ~= "string") then
                    error("Argument #2 is not a string!")
                    return false
                end

                if (type(func) ~= "function") then
                    error("Argument #3 is not a function!")
                    return false
                end

                if (origRegister(callback, name, func)) then
                    table.insert(callbackList, {tostring(callback), tostring(name)})
                    sendCallbackList()
                    return true
                end
            end
        else
            error("Insuficient number of arguments!")
        end

        return false
    end,

    Unregister = function(callback, name)
        if not (type(callback) == "string") then
            error("Argument #1 is not a string!")
            return false
        end

        if not (type(name) == "string") then
            error("Argument #2 is not a string!")
            return false
        end

        for i, call in ipairs(callbackList) do
            if (call[1] == callback and call[2] == name) then
                if (origUnregister(callback, tostring(name))) then
                    table.remove(callbackList, i)
                    sendCallbackList()
                    return
                end
            end
        end

        return false
    end
}

local function poll()
    local script = http.Get("http://localhost:8080/getcurrentscript")
    if (script ~= "") then
        local func, err = load(script, nil, "t", env)
        if (func) then
            pcall(func)
        else
            error(tostring(err))
            print(debug.traceback())
        end
    end
end

if not env._runner_initialized then
    origRegister("Draw", "LuaRunnerDraw", poll)
    origRegister("Unload", "LuaRunnerUnload", function()
        for _, call in ipairs(callbackList) do
            env.callbacks.Unregister(call[1], call[2])
        end
        env.callbacks.Register = origRegister
        env.callbacks.Unregister = origUnregister
        env._runner_initialized = nil
    end)
    env._runner_initialized = true
end
