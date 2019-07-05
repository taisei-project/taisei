
-- BEGIN IMPORTS

local assert = assert
local error = error
local getmetatable = getmetatable
local rawget = rawget
local setmetatable = setmetatable

local util = require 'api.util'
local string = require 'string'
local table = require 'table'
local debug = require 'debug'

local debug_getmetatable = debug.getmetatable

local E = require 'engine'

local RESF_DEFAULT = assert(E.RESF_DEFAULT)
local RESF_LAZY = assert(E.RESF_LAZY)
local RESF_OPTIONAL = assert(E.RESF_OPTIONAL)
local RES_ANIMATION = (E.RES_ANIMATION)
local RES_FONT = assert(E.RES_FONT)
local RES_MODEL = assert(E.RES_MODEL)
local RES_MUSIC = assert(E.RES_MUSIC)
local RES_MUSICMETA = assert(E.RES_MUSICMETA)
local RES_POSTPROCESS = assert(E.RES_POSTPROCESS)
local RES_SHADEROBJ = assert(E.RES_SHADEROBJ)
local RES_SHADERPROG = assert(E.RES_SHADERPROG)
local RES_SOUND = assert(E.RES_SOUND)
local RES_SPRITE = assert(E.RES_SPRITE)
local RES_STATUS_EXTERNAL = assert(E.RES_STATUS_EXTERNAL)
local RES_STATUS_FAILED = assert(E.RES_STATUS_FAILED)
local RES_STATUS_LOADED = assert(E.RES_STATUS_LOADED)
local RES_STATUS_LOADING = assert(E.RES_STATUS_LOADING)
local RES_STATUS_NOT_LOADED = assert(E.RES_STATUS_NOT_LOADED)
local RES_TEXTURE = assert(E.RES_TEXTURE)
local animation_get_sequence_frame = assert(E.animation_get_sequence_frame)
local animation_get_sequence_length = assert(E.animation_get_sequence_length)
local animation_get_sequences = assert(E.animation_get_sequences)
local animation_get_sprite = assert(E.animation_get_sprite)
local animation_get_sprite_count = assert(E.animation_get_sprite_count)
local res_ref = assert(E.res_ref)
local res_ref_copy = assert(E.res_ref_copy)
local res_ref_data = assert(E.res_ref_data)
local res_ref_id = assert(E.res_ref_id)
local res_ref_name = assert(E.res_ref_name)
local res_ref_status = assert(E.res_ref_status)
local res_ref_type = assert(E.res_ref_type)
local res_ref_wait_ready = assert(E.res_ref_wait_ready)
local res_refs_are_equivalent = assert(E.res_refs_are_equivalent)
local res_unref = assert(E.res_unref)
local res_unref_if_valid = assert(E.res_unref_if_valid)
local sprite_get_extent = assert(E.sprite_get_extent)
local sprite_get_offset = assert(E.sprite_get_offset)
local sprite_get_texarea_extent = assert(E.sprite_get_texarea_extent)
local sprite_get_texarea_offset = assert(E.sprite_get_texarea_offset)
local sprite_get_texture_ref = assert(E.sprite_get_texture_ref)

-- END IMPORTS

-- BEGIN MODULE LOCALS

local resources = {}
local cookie = {}
local wrapper_to_ref = setmetatable({}, { __mode = 'k' })

local status_strings = {
	[RES_STATUS_EXTERNAL] = 'external',
	[RES_STATUS_FAILED] = 'failed',
	[RES_STATUS_LOADED] = 'loaded',
	[RES_STATUS_LOADING] = 'loading',
	[RES_STATUS_NOT_LOADED] = 'not loaded',
}

-- END MODULE LOCALS

-- BEGIN UTILS

local function make_metatable(name)
	return {
		__name = name,
		__metatable = name,
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

local function is_resource_ref(self)
	local mt = debug_getmetatable(self)
	return mt and (mt.cookie == cookie)
end

local function get_ref(self)
	assert(is_resource_ref(self), 'not a ResourceRef value')
	return wrapper_to_ref[self]
end

local function make_proxy(self, proxymt)
	local obj = {}
	wrapper_to_ref[obj] = get_ref(self)
	return setmetatable(obj, proxymt)
end

local function wrap_ref(raw_ref, metatable, copy_needed)
	local obj = {}

	if copy_needed then
		wrapper_to_ref[obj] = res_ref_copy(raw_ref)
	else
		wrapper_to_ref[obj] = raw_ref
	end

	return setmetatable(obj, metatable)
end

local function make_res_func(rtype, metatable)
	return function(name, flags)
		return wrap_ref(res_ref(rtype, name, flags or RESF_DEFAULT), metatable)
	end
end

-- END UTILS

local ResourceRefBase = make_metatable('ResourceRefBase')
do
	function ResourceRefBase:__index(key)
		local mt = debug_getmetatable(self)
		local getter = mt.getters[key]

		if getter then
			return getter(self, key)
		end

		return mt.methods[key]
	end

	function ResourceRefBase:__newindex()
		error('attempt to modify a ' .. debug_getmetatable(self).__name .. ' value', 2)
	end
end

local ResourceRef = inherit(ResourceRefBase, 'ResourceRef')
do
	function ResourceRef:__gc()
		res_unref_if_valid(get_ref(self))
	end

	function ResourceRef:__eq(other)
		return
			is_resource_ref(self) and
			is_resource_ref(other) and
			res_refs_are_equivalent(
				wrapper_to_ref[self],
				wrapper_to_ref[other]
			)
	end

	function ResourceRef:__tostring()
		return string.format('%s `%s` (%s): %p',
			debug_getmetatable(self).__name,
			self.name,
			status_strings[self.status],
			self
		)
	end

	function ResourceRef.getters:type()
		return res_ref_type(get_ref(self))
	end

	function ResourceRef.getters:name()
		return res_ref_name(get_ref(self))
	end

	function ResourceRef.getters:status()
		return res_ref_status(get_ref(self))
	end

	function ResourceRef.getters:id()
		return res_ref_id(get_ref(self))
	end

	function ResourceRef.methods:wait_ready()
		return res_ref_wait_ready(get_ref(self))
	end
end

local TextureRef = inherit(ResourceRef, 'TextureRef')
do
end

local SpriteRef = inherit(ResourceRef, 'SpriteRef')
do
	local SpriteRefTextureAreaProxy = inherit(ResourceRefBase, 'SpriteRefTextureAreaProxy')

	function SpriteRef.getters:offset()
		return sprite_get_offset(res_ref_data(get_ref(self)))
	end

	function SpriteRef.getters:extent()
		return sprite_get_extent(res_ref_data(get_ref(self)))
	end

	function SpriteRef.getters:texture()
		return wrap_ref(sprite_get_texture_ref(res_ref_data(get_ref(self))), TextureRef, true)
	end

	function SpriteRef.getters:texture_area()
		return make_proxy(self, SpriteRefTextureAreaProxy)
	end

	function SpriteRefTextureAreaProxy.getters:offset()
		return sprite_get_texarea_offset(res_ref_data(get_ref(self)))
	end

	function SpriteRefTextureAreaProxy.getters:extent()
		return sprite_get_texarea_extent(res_ref_data(get_ref(self)))
	end
end

local AnimationRef = inherit(ResourceRef, 'AnimationRef')
do
	local AnimationRefSequencesProxy = inherit(ResourceRefBase, 'AnimationRefSequencesProxy')
	local AnimationRefSequenceProxy = inherit(ResourceRefBase, 'AnimationRefSequenceProxy')

	local ResourceRef_index = ResourceRef.__index
	local ResourceRefBase_index = ResourceRefBase.__index

	function AnimationRef:__index(key)
		if type(key) ~= 'number' then
			return ResourceRef_index(self, key)
		end

		local ani = res_ref_data(get_ref(self))

		-- ipairs doesn't work without this
		if key > animation_get_sprite_count(ani) or key < 1 then
			return nil
		end

		return wrap_ref(animation_get_sprite(ani, key), SpriteRef, true)
	end

	function AnimationRef:__len()
		return animation_get_sprite_count(res_ref_data(get_ref(self)))
	end

	function AnimationRef.getters:sequences()
		return make_proxy(self, AnimationRefSequencesProxy)
	end

	AnimationRef.__pairs = ipairs

	function AnimationRefSequencesProxy:__index(key)
		local seq = make_proxy(self, AnimationRefSequenceProxy)
		rawset(seq, 'sequence_id', key)
		return seq
	end

	function AnimationRefSequencesProxy:__pairs()
		local ani = res_ref_data(get_ref(self))
		local seq_ids = animation_get_sequences(ani)
		local index = 1

		local function iterator()
			local key = seq_ids[index]

			if key then
				index = index + 1
				return key, self[key]
			end
		end

		return iterator
	end

	function AnimationRefSequenceProxy:__index(key)
		if type(key) ~= 'number' then
			return ResourceRefBase_index(self, key)
		end

		local ani = res_ref_data(get_ref(self))
		local seq_id = rawget(self, 'sequence_id')

		-- ipairs doesn't work without this
		if key > animation_get_sequence_length(ani, seq_id) or key < 1 then
			return nil
		end

		local f_idx, f_mirrored, f_ref = animation_get_sequence_frame(ani, seq_id, key)

		return {
			index = f_idx,
			sprite = wrap_ref(f_ref, SpriteRef, true),
			is_mirrored = f_mirrored,
		}
	end

	function AnimationRefSequenceProxy:__len()
		local ani = res_ref_data(get_ref(self))
		local seq_id = rawget(self, 'sequence_id')
		return animation_get_sequence_length(ani, seq_id)
	end

	AnimationRefSequenceProxy.__pairs = ipairs
end

function resources.status_string(status)
	return status_strings[status]
end

return table.meld(resources, {
	FLAGS_DEFAULT = RESF_DEFAULT,
	FLAG_LAZY = RESF_LAZY,
	FLAG_OPTIONAL = RESF_OPTIONAL,
	STATUS_EXTERNAL = RES_STATUS_EXTERNAL,
	STATUS_FAILED = RES_STATUS_FAILED,
	STATUS_LOADED = RES_STATUS_LOADED,
	STATUS_LOADING = RES_STATUS_LOADING,
	STATUS_NOT_LOADED = RES_STATUS_NOT_LOADED,
	TYPE_ANIMATION = RES_ANIMATION,
	TYPE_FONT = RES_FONT,
	TYPE_MODEL = RES_MODEL,
	TYPE_MUSIC = RES_MUSIC,
	TYPE_MUSICMETA = RES_MUSICMETA,
	TYPE_POSTPROCESS = RES_POSTPROCESS,
	TYPE_SHADEROBJ = RES_SHADEROBJ,
	TYPE_SHADERPROG = RES_SHADERPROG,
	TYPE_SOUND = RES_SOUND,
	TYPE_SPRITE = RES_SPRITE,
	TYPE_TEXTURE = RES_TEXTURE,
	animation = make_res_func(RES_ANIMATION, AnimationRef),
	sprite = make_res_func(RES_SPRITE, SpriteRef),
	texture = make_res_func(RES_TEXTURE, TextureRef),
})
