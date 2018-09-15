
# Thanks, google.
# https://github.com/google/shaderc/issues/392
if get_option('static')
    dep_shaderc = cc.find_library('shaderc_combined', required: true)
else
    dep_shaderc = cc.find_library('shaderc_shared', required: true)
endif

dep_crossc = subproject('crossc').get_variable('crossc_dep')

r_spirv_tools_src = files(
    'shader_spirv.c'
)

r_spirv_tools_libdeps = [dep_shaderc, dep_crossc]
