project('glad', 'c')

glad_api = ['gl:core=3.3', 'gles2=3.0', 'egl=1.5']
glad_extensions = [
    'GL_3DFX_texture_compression_FXT1',
    'GL_AMD_compressed_ATC_texture',
    'GL_ANGLE_depth_texture',
    'GL_ANGLE_instanced_arrays',
    'GL_ANGLE_texture_compression_dxt3',
    'GL_ANGLE_texture_compression_dxt5',
    'GL_ANGLE_translated_shader_source',
    'GL_ARB_ES3_compatibility',
    'GL_ARB_clear_texture',
    'GL_ARB_debug_output',
    'GL_ARB_depth_texture',
    'GL_ARB_draw_buffers',
    'GL_ARB_draw_instanced',
    'GL_ARB_instanced_arrays',
    'GL_ARB_internalformat_query2',
    'GL_ARB_pixel_buffer_object',
    'GL_ARB_texture_compression_bptc',
    'GL_ARB_texture_compression_rgtc',
    'GL_ARB_texture_filter_anisotropic',
    'GL_ARB_vertex_array_object',
    'GL_ARB_viewport_array',
    'GL_ATI_draw_buffers',
    'GL_EXT_clear_texture',
    'GL_EXT_color_buffer_float',
    'GL_EXT_draw_buffers',
    'GL_EXT_draw_instanced',
    'GL_EXT_float_blend',
    'GL_EXT_instanced_arrays',
    'GL_EXT_pixel_buffer_object',
    'GL_EXT_pvrtc_sRGB',
    'GL_EXT_sRGB',
    'GL_EXT_texture_compression_bptc',
    'GL_EXT_texture_compression_dxt1',
    'GL_EXT_texture_compression_rgtc',
    'GL_EXT_texture_compression_s3tc',
    'GL_EXT_texture_compression_s3tc_srgb',
    'GL_EXT_texture_filter_anisotropic',
    'GL_EXT_texture_norm16',
    'GL_EXT_texture_rg',
    'GL_EXT_texture_sRGB',
    'GL_EXT_texture_sRGB_R8',
    'GL_EXT_texture_sRGB_RG8',
    'GL_IMG_texture_compression_pvrtc',
    'GL_IMG_texture_compression_pvrtc2',
    'GL_KHR_debug',
    'GL_KHR_texture_compression_astc_ldr',
    'GL_NV_draw_instanced',
    'GL_NV_instanced_arrays',
    'GL_NV_pixel_buffer_object',
    'GL_NV_sRGB_formats',
    'GL_NV_viewport_array2',
    'GL_OES_compressed_ETC1_RGB8_texture',
    'GL_OES_depth_texture',
    'GL_OES_texture_compression_astc',
    'GL_OES_texture_float_linear',
    'GL_OES_texture_half_float_linear',
    'GL_OES_vertex_array_object',
    'GL_OES_viewport_array',
    'GL_SGIX_depth_texture',
]

glad_program = find_program('glad', required : false)

glad_args = [
    '--api', ','.join(glad_api),
    '--extensions', ','.join(glad_extensions),
    '--merge',
    '--out-path', meson.current_source_dir(),
    'c',
    '--alias',
]

glad_sources = []
glad_includes = include_directories('include')

glad_gl_lib = static_library('gladGL', files('src/gl.c'),
    install : not meson.is_subproject(),
    build_by_default : not meson.is_subproject(),
    include_directories : glad_includes,
)

glad_egl_lib = static_library('gladEGL', files('src/egl.c'),
    install : not meson.is_subproject(),
    build_by_default : not meson.is_subproject(),
    include_directories : glad_includes,
)

glad_gl_dep = declare_dependency(
  include_directories : glad_includes,
  link_with : glad_gl_lib
)

glad_egl_dep = declare_dependency(
  include_directories : glad_includes,
  link_with : glad_egl_lib
)

if glad_program.found()
    run_target('gen-gl-loader',
        command : [glad_program, glad_args],
    )
endif
