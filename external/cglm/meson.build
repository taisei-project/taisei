project('cglm', 'c',
  license : 'MIT',
  version : '0.3.3',
  default_options : [
    'c_std=c99',
    'warning_level=3',
  ]
)

cglm_args = []
cglm_sources = []
cglm_headers = []
cglm_includes = []
cglm_links = []
cglm_deps = []

# Hiding symbols that are not explicitly marked as exported
# requires a compiler flag on all compilers except VS.
cc = meson.get_compiler('c')
if cc.get_id() != 'msvc'
  cglm_args += ['-fvisibility=hidden']
endif

headers_subdir = meson.project_name()

cglm_deps += cc.find_library('m', required : false)

subdir('src')
subdir('include')

lib_type = get_option('lib_type')

if lib_type == 'default_library'
  lib_type = get_option('default_library')
endif

if lib_type == 'static'
  cglm_lib = static_library('cglm', cglm_sources,
    install : not meson.is_subproject(),
    build_by_default : not meson.is_subproject(),
    c_args : cglm_args,
    dependencies: cglm_deps,
  )
elif lib_type == 'shared'
  cglm_lib = static_library('cglm', cglm_sources,
    install : not meson.is_subproject(),
    build_by_default : not meson.is_subproject(),
    c_args : cglm_args,
    dependencies: cglm_deps,
  )
elif lib_type == 'inline'
  cglm_lib = []
endif

# Make this library usable as a Meson subproject.
cglm_dep = declare_dependency(
  include_directories: cglm_includes,
  link_with : cglm_lib
)

if not meson.is_subproject()
  pkg_mod = import('pkgconfig')
  pkg_mod.generate(
    name : meson.project_name(),
    filebase : meson.project_name(),
    description : 'Optimized OpenGL/Graphics Math (glm) for C.',
    # subdirs : meson.project_name(),
    libraries : [cglm_lib],
    version : meson.project_version(),
  )
endif
