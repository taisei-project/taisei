
print('Hello, Lua!')

do
	--[[
		Provide a 'require' wrapper that normalizes the module name and resolves relative imports.
	--]]

	local function normalizemodname(name)
		local badchar = name:match('[^%.%w_]')
		if badchar then error("invalid character "..badchar.." in module name '"..name.."'") end

		local elems = table.collect(name:gmatch('[^%.]+'))
		local isrelative = name:sub(1, 1) == '.'

		name = table.concat(elems, '.')

		if isrelative then
			-- Traverse the call stack to get the caller's file name
			-- We'll use it to resolve the relative module name into an absolute one

			-- We start inspecting from position 3, because:
			-- 0 = debug.getinfo itself
			-- 1 = this function
			-- 2 = 'require' wrapper
			-- 3 = caller of 'require'
			local i = 3
			local caller_filename

			-- FIXME: Tail calls can mess this up, but it's not important to support them,
			--        so perhaps we should just error out if we encounter one.

			repeat
				local info = assert(debug.getinfo(i, 'S'), "relative import of "..name.." impossible in this context")
				i = i + 1

				if info.source:sub(1, 1) == '@' then
					caller_filename = info.source:sub(2)
				end
			until caller_filename

			local iter = caller_filename:gmatch('[^/]+')
			iter()  -- eat 'res'
			iter()  -- eat 'scripts'
			local pentries = table.collect(iter)
			pentries[#pentries] = name
			name = table.concat(pentries, '.')
		end

		return name
	end

	local real_require = require

	function require(name, ...)
		return real_require(normalizemodname(name), ...)
	end
end

do
	--[[
		Set up a VFS-based searcher for the require() function
	--]]

	local preload_searcher = package.searchers[1]
	package.searchers = { preload_searcher, function(name)
		local path = 'res/scripts/' .. name:gsub('%.+', '/') .. '.lua'
		local file, err = vfs.open(path)
		if not file then return "\n\t" .. err end
		return assert(load(file:read(), '@' .. path, 't'))
	end }
end

do
	--[[
		Some useful utilities
	--]]

	function table.collect(iterator, state, var)
		local t = {}
		while true do
			var = iterator(state, var)
			if var == nil then break end
			t[#t + 1] = var
		end
		return t
	end
end

do
	--[[
		Some console shortcuts
	--]]

	function ls(path)
		for p in assert(vfs.iterdir(path)) do
			print(p)
		end
	end

	function cat(path)
		print(assert(vfs.open(path)):read())
	end

	function p(t) for k, v in pairs(t) do print(("%24s"):format(k), v) end end
end

do
	--[[
		Try to execute the local init script
	--]]

	local ok, result = pcall(require, 'local_init')
	if not ok then print(result) end
end
