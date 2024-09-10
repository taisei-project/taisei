
r_shaderlib_src = files(
    'cache.c',
    'lang_dxbc.c',
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
dep_shaderc = dependency('shaderc', required : opt_shader_transpiler)
shader_transpiler_enabled = dep_spvc.found() and dep_shaderc.found()

opt_shader_transpiler_dxbc = (get_option('shader_transpiler_dxbc').
    disable_auto_if(host_machine.system() != 'windows').
    require(shader_transpiler_enabled,
        error_message : 'shader_transpiler must be enabled to use shader_transpiler_dxbc'))

if shader_transpiler_enabled
    r_shaderlib_src += files('lang_spirv.c')
    r_shaderlib_libdeps += [dep_shaderc, dep_spvc]
else
    r_shaderlib_src += files('lang_spirv_stub.c')
endif

if opt_shader_transpiler_dxbc.allowed()
    r_shaderlib_src += files('lang_dxbc.c')
else
    r_shaderlib_src += files('lang_dxbc_stub.c')
endif
