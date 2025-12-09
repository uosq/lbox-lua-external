--- made by navet
--- helper script for Lua Runner

local failCount = 0

local function unload()
    callbacks.Unregister("Draw", "LuaRunnerDraw")
end

local function poll()
	local script = http.Get("http://localhost:8008/getcurrentscript")
	if (script == "") then
		failCount = failCount + 1
		if failCount >= 3 then
			printc(255, 150, 150, 255, "Failed " .. failCount .. " times! Unloading...")
			unload()
		end

		return

	elseif script == "empty" then
		return
	end

	local func, err = load(script)
	if (func) then
		pcall(func)
	else
		error(tostring(err))
	end
end

callbacks.Unregister("Draw", "LuaRunnerDraw")
callbacks.Register("Draw", "LuaRunnerDraw", poll)
