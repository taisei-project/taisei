
r_shaderlib_src = files(
    'cache.c',
    'lang_glsl.c',
    'lang_spirv_aux.c',
    'shaderlib.c',
)

r_shaderlib_libdeps = []

if get_option('shader_transpiler')
    dep_spvc = dependency('spirv-cross-c-shared', required : true, static : static, fallback : ['SPIRV-Cross', 'spirv_cross_dep'])
    dep_shaderc = dependency('shaderc', static : static, fallback : ['shaderc', 'shaderc_dep'], required : true)

    r_shaderlib_src += files(
        'lang_spirv.c'
    )

    r_shaderlib_libdeps += [dep_shaderc, dep_spvc]

    if false and host_machine.system() == 'windows' and get_option('b_lto') and get_option('static') and cc.get_id() == 'gcc'
        warning('LTO is known to break glslang on GCC-mingw64 based toolchains')
    endif
else
    r_shaderlib_src += files(
        'lang_spirv_stub.c'
    )
endif
