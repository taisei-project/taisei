
-- BEGIN IMPORTS

local getmetatable = getmetatable
local setmetatable = setmetatable
local type = type

local table = table

-- END IMPORTS

do -- monkey-patch standard library
	function table.collect(iterator, state, var)
		local t = {}
		while true do
			var = iterator(state, var)
			if var == nil then break end
			t[#t + 1] = var
		end
		return t
	end

	function table.collectpairs(iterator, state, key)
		local t = {}
		while true do
			key, val = iterator(state, key)
			if key == nil then break end
			t[key] = val
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
end -- monkey-patch standard library

local util = {}

function util.type_name(value)
	local n = type(value)
	local mt = getmetatable(value)

	-- protected metatables return type name by convention
	if type(mt) == 'string' then return mt end

	return mt and mt.__name or n
end

return util
