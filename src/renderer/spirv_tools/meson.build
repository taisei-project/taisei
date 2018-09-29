
# Thanks, google.
# https://github.com/google/shaderc/issues/392
if get_option('static')
    dep_shaderc = cc.find_library('shaderc_combined', required: true)
else
    dep_shaderc = cc.find_library('shaderc_shared', required: true)
endif

dep_crossc = dependency('crossc',
    required : true,
    static : get_option('static'),
    version : '>=1.5.0',
    fallback : ['crossc', 'crossc_dep']
)

r_spirv_tools_src = files(
    'shader_spirv.c'
)

r_spirv_tools_libdeps = [dep_shaderc, dep_crossc]

if host_machine.system() == 'windows' and get_option('b_lto') and get_option('static')
    error(
'''

 -!-  LTO is known to break glslang on Windows.
 -!-  Please disable it with `meson configure -Db_lto=false`.
 -!-  If you *really* want to use LTO, force it via compiler args or patch this check out.
''')
endif
