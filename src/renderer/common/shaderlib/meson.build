
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
