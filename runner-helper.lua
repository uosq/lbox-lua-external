filesystem.CreateDirectory("Executor")

local scriptFilePath = "Executor/script.lua"

local function Draw(uCmd)
    -- Check if the script file exists
    local file = io.open(scriptFilePath, "r")
    if file then
        file:setvbuf("no")

        -- Read the entire file
        local code = file:read("*all")
        file:close()

        -- Run the code safely
        local func, err = load(code)
        if func then
            pcall(func)
        else
            print("Error loading script: " .. tostring(err))
        end

        -- Delete the file after running
        os.remove(scriptFilePath)
    end
end

callbacks.Register("Draw", Draw)
