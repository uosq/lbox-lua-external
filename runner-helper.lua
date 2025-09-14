filesystem.CreateDirectory("Executor")

local scriptFilePath = "Executor/script.lua"
local consoleFilePath = "Executor/console.txt"

local env = _ENV

env.print = function(...)
    local console <close> = io.open(consoleFilePath, "a+")
    if console then
        console:setvbuf("no")
        local args = {...}
        for _, line in pairs (args) do
            console:write(string.format('%s\n', tostring(line)))
        end
        console:flush()
    end
end

env.error = function(...)
    local args = {...}
    for i = 1, #args do
        if (args[i]) then
            env.print(string.format('Error: %s', tostring(args[i])))
        end
    end
end

env.warn = function(...)
    local args = {...}
    for i = 1, #args do
        if (args[i]) then
            env.print(string.format('Warn: %s', tostring(args[i])))
        end
    end
end

local function Draw(uCmd)
    -- Check if the script file exists
    local file = io.open(scriptFilePath, "r")
    if file then
        file:setvbuf("no")

        -- Read the entire file
        local code = file:read("*all")
        file:close()

        -- Run the code safely
        local func, err = load(code, nil, "t", env)
        if func then
            pcall(func)
        else
            --print("Error loading script: " .. tostring(err))
            local console <close> = io.open(consoleFilePath, "a+")
            if console then
                console:setvbuf("no")
                console:write(string.format("%s\n", tostring(err)))
                console:flush()
            end
        end

        -- Delete the file after running
        os.remove(scriptFilePath)
    end
end

callbacks.Register("Draw", Draw)