
r_shaderlib_src = files(
    'cache.c',
    'lang_glsl.c',
    'lang_spirv_aux.c',
    'reflect.c',
    'shaderlib.c',
)

r_shaderlib_libdeps = []

# NOTE: transpile_glsl is for static, offline translation.
# It makes little sense to include the transpiler if that is used.
opt_shader_transpiler = get_option('shader_transpiler').disable_auto_if(transpile_glsl)

# We have to test for Taint ealy, because if we have to build it, it will pull in the SPIRV-Tools subproject, even if
# SPIRV-Tools is installed. That's because of Tint abusing the SPIRV-Tools-opt internals for its SPIR-V frontend. Thank
# google for this mess.
# If we end up building glslang too, we want it to find the same version of SPIRV-Tools instead of linking to yet
# another one from the system.
opt_shader_transpiler_wgsl = (get_option('shader_transpiler_wgsl').
    require(not opt_shader_transpiler.disabled(),
        error_message : 'shader_transpiler must be enabled to use shader_transpiler_wgsl'))

dep_taint = dependency('taint',
    required : opt_shader_transpiler_wgsl,
    fallback : ['taint', 'taint_dep'])

dep_spvc = dependency('spirv-cross-c-shared', required : opt_shader_transpiler)
dep_glslang = dependency('glslang',
    required : opt_shader_transpiler,
    fallback : ['glslang', 'glslang_dep'],
    method : 'cmake',
    modules : ['glslang::glslang', 'glslang::glslang-default-resource-limits'])
shader_transpiler_enabled = dep_spvc.found() and dep_glslang.found()

opt_shader_transpiler_dxbc = (get_option('shader_transpiler_dxbc').
    disable_auto_if(host_machine.system() != 'windows').
    require(shader_transpiler_enabled,
        error_message : 'shader_transpiler must be enabled to use shader_transpiler_dxbc'))

opt_shader_transpiler_wgsl = (opt_shader_transpiler_wgsl.
    require(shader_transpiler_enabled,
        error_message : 'shader_transpiler must be enabled to use shader_transpiler_wgsl'))

if shader_transpiler_enabled
    r_shaderlib_src += files('lang_spirv.c')
    r_shaderlib_libdeps += [dep_glslang, dep_spvc]
else
    r_shaderlib_src += files('lang_spirv_stub.c')
endif

if opt_shader_transpiler_dxbc.allowed()
    r_shaderlib_src += files('lang_dxbc.c')
else
    r_shaderlib_src += files('lang_dxbc_stub.c')
endif

if dep_taint.found() and opt_shader_transpiler_wgsl.allowed()
    r_shaderlib_src += files('lang_wgsl.c')
    r_shaderlib_libdeps += [dep_taint]
else
    r_shaderlib_src += files('lang_wgsl_stub.c')
endif
