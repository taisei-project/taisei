project(
  'naga-wrapper', 'c',
  meson_version : '>= 1.3.0',
)

# Unfortunately Naga and/or a bunch of its dependencies use build.rs, so they can't be integrated with meson's
# nice rust module. We have to resort to this unholy abomination instead. I'm truly sorry.
# TODO maybe vendor everything, so that at least cargo doesn't download anything during the build.

# Make sure cargo works
cargo = find_program('cargo', required : true)
run_command([cargo, 'clean', '--dry-run', '--manifest-path', files('Cargo.toml')], check : true)

lib_name = 'libwgslcrap'
build_wrapper = find_program('./build-wrapper.py')

if get_option('optimization') in ['1', '2', '3', 's']
    profile = 'release'

    if get_option('b_lto')
        profile += '-lto'
    endif

    if get_option('debug')
        profile += '-debug'
    endif
else
    profile = 'dev'
endif

build_cmd = [build_wrapper,
  '@PRIVATE_DIR@',
  profile,
  '--lib-output', '@OUTPUT@',
  '--depfile', '@DEPFILE@',
  '--lib-name', 'libwgslcrap',
]

if meson.is_cross_build()
    build_cmd += ['--rust-target', meson.get_external_property('rust_target', native : false)]
endif

lib = custom_target(
  command : build_cmd,
  output  : f'@lib_name@.a',
  depfile : f'@lib_name@.d',
  console : true,
  install : false,
  build_by_default : true
)

wgslcrap_dep = declare_dependency(
  include_directories : include_directories('include'),
  link_with : lib,
)

meson.override_dependency('wgslcrap', wgslcrap_dep)

executable('wgslify', 'wgslify.c',
  dependencies : wgslcrap_dep,
  override_options : {
    'c_std' : 'gnu23',
  },
  build_by_default : not meson.is_subproject(),
  install : false,
)
