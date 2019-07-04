
print('Hello, Lua!')

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

	function table.copy(t)
		local newt = {}

		for k, v in pairs(t) do
			newt[k] = v
		end

		return newt
	end

	function table.meld(dest, src)
		for k, v in pairs(src) do
			dest[k] = v
		end

		return dest
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

api = require('api')

do
	--[[
		Try to execute the local init script
	--]]

	-- local ok, result = pcall(require, 'local_init')
	-- if not ok then print(result) end
	require 'local_init'
end
