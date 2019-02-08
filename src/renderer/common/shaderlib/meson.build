
r_shaderlib_src = files(
    'cache.c',
    'lang_glsl.c',
    'lang_spirv_aux.c',
)

r_shaderlib_libdeps = []

if get_option('shader_transpiler')
    assert(dep_shaderc.found() and dep_crossc.found(),
        'shaderc and crossc are required for shader translation. Install them or disable shader_transpiler.'
    )

    r_shaderlib_src += files(
        'lang_spirv.c'
    )

    r_shaderlib_libdeps += [dep_shaderc, dep_crossc]

    if host_machine.system() == 'windows' and get_option('b_lto') and get_option('static')
        error(
    '''

    -!-  LTO is known to break glslang on Windows.
    -!-  Please disable it with `meson configure -Db_lto=false`.
    -!-  If you *really* want to use LTO, force it via compiler args or patch this check out.
    ''')
    endif
else
    r_shaderlib_src += files(
        'lang_spirv_stub.c'
    )
endif
