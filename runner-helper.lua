--- made by navet

local lastCheck = 0

local function poll()
    if globals.RealTime() - lastCheck < 0.25 then
        return
    end

    lastCheck = globals.RealTime()

    local file = io.open("Executor/script.lua", "r")
    if file then
        local code = file:read("*all")
        file:close()

        local func, err = load(code)
        if func then
            pcall(func)
        else
            print(tostring(err))
        end

        os.remove("Executor/script.lua")
    end
end

callbacks.Register("Draw", poll)
