
local rawget = rawget
local getmetatable = getmetatable

local E = require 'engine'

local RESF_DEFAULT = E.RESF_DEFAULT
local RES_SPRITE = E.RES_SPRITE
local res_ref = assert(E.res_ref)
local res_ref_data = assert(E.res_ref_data)
local res_ref_name = assert(E.res_ref_name)
local res_ref_status = assert(E.res_ref_status)
local res_ref_wait_ready = assert(E.res_ref_wait_ready)
local res_unref_if_valid = assert(E.res_unref_if_valid)
local sprite_get_extent = assert(E.sprite_get_extent)
local sprite_get_offset = assert(E.sprite_get_offset)
local sprite_get_texarea_extent = assert(E.sprite_get_texarea_extent)
local sprite_get_texarea_offset = assert(E.sprite_get_texarea_offset)
local res_ref_type = assert(E.res_ref_type)

local cookie = {}

local function make_metatable(name)
	return {
		__name = name,
		cookie = cookie,
		methods = {},
		getters = {},
	}
end

local function inherit(base, name)
	local mt = table.copy(base)
	mt.__name = name
	mt.methods = table.copy(base.methods)
	mt.getters = table.copy(base.getters)
	return mt
end

local function getref(self)
	assert(getmetatable(self).cookie == cookie, 'not a ResourceRef value')
	return rawget(self, '__ref')
end

local function make_proxy(self, proxymt)
	return setmetatable({
		__ref = getref(self),
		__obj = self,
	}, proxymt)
end

local ResourceRefBase = make_metatable('ResourceRefBase')
do
	function ResourceRefBase:__index(key)
		local mt = getmetatable(self)
		local getter = mt.getters[key]

		if getter then
			return getter(self, key)
		end

		return mt.methods[key]
	end

	function ResourceRefBase:__newindex()
		error('attempt to modify a ' .. getmetatable(self).__name .. ' value')
	end
end

local ResourceRef = inherit(ResourceRefBase, 'ResourceRef')
do
	function ResourceRef:__gc()
		ref_unref_if_valid(getref(self))
	end

	function ResourceRef.getters:type()
		return res_ref_type(getref(self))
	end

	function ResourceRef.getters:name()
		return res_ref_name(getref(self))
	end

	function ResourceRef.getters:status()
		return res_ref_status(getref(self))
	end

	function ResourceRef.methods:wait_ready()
		return res_ref_wait_ready(getref(self))
	end

	ResourceRef.__close = ResourceRef.__gc
end

local SpriteRef = inherit(ResourceRef, 'SpriteRef')
do
	local SpriteRefTextureAreaProxy = inherit(ResourceRefBase, 'SpriteRefTextureAreaProxy')

	function SpriteRef.getters:offset()
		return sprite_get_offset(res_ref_data(getref(self)))
	end

	function SpriteRef.getters:extent()
		return sprite_get_extent(res_ref_data(getref(self)))
	end

	function SpriteRef.getters:texture_area()
		return make_proxy(self, SpriteRefTextureAreaProxy)
	end

	function SpriteRefTextureAreaProxy.getters:offset()
		return sprite_get_texarea_offset(res_ref_data(getref(self)))
	end

	function SpriteRefTextureAreaProxy.getters:extent()
		return sprite_get_texarea_extent(res_ref_data(getref(self)))
	end
end

local function new_ref(rtype, metatable, name, flags)
	local ref = res_ref(rtype, name, flags or RESF_DEFAULT)
	return setmetatable({ __ref = ref }, metatable)
end

local function make_res_func(rtype, metatable)
	return function(name, flags)
		return new_ref(rtype, metatable, name, flags)
	end
end

return {
	sprite = make_res_func(RES_SPRITE, SpriteRef),
}
