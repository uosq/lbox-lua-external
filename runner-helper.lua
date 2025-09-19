--- made by navet
--- helper script for Lua Runner

local function poll()
    local script = http.Get("http://localhost:8080/")
    if (script ~= "") then
        local func, err = load(script)
        if (func) then
            pcall(func)
        else
            local console = io.open("Executor/console.txt", "a+")
            if (console) then
                console:write(tostring(err))
                console:flush()
                console:close()
            end
        end
    end
end

callbacks.Register("Draw", poll)
