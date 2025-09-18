--- made by navet

filesystem.CreateDirectory("Executor")
filesystem.CreateDirectory("Executor/Scripts")

local lastCheck = 0

local function poll()
    local diff = (globals.RealTime() - lastCheck)
    if diff < 0.25 then
        return
    end

    lastCheck = globals.RealTime()

    local filesToRemove = {}

    filesystem.EnumerateDirectory("Executor/Scripts/*.lua", function(fileName)
        local file = io.open(string.format("Executor/Scripts/%s", fileName), "r")
        if file then
            local code = file:read("*all")
            file:close()

            local func, err = load(code)
            if func then
                pcall(func)
            else
                local console = io.open("Executor/console.txt", "a+")
                if console then
                    console:write(tostring(err))
                    console:flush()
                    console:close()
                end
            end

            --os.remove(fileName)
            filesToRemove[#filesToRemove+1] = fileName
        end
    end)

    for _, name in ipairs(filesToRemove) do
        os.remove(string.format("Executor/Scripts/%s", name))
    end
end

callbacks.Register("Draw", poll)
