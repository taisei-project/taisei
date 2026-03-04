
resources_dir = meson.current_source_dir()

packages = [
    '00-taisei',
]

res_install_tag = 'resources'
use_static_res_index = (host_machine.system() == 'emscripten')

foreach pkg : packages
    pkg_pkgdir = '@0@.pkgdir'.format(pkg)
    subdir(pkg_pkgdir)
endforeach

if use_static_res_index
    resindex_deps = []
    resindex_cmd = [
        index_resources_command,
        '@OUTPUT@',
    ]

    resindex_cmd += ['--subdir', 'locale', l10n_build_dir]
    resindex_deps += l10n_mo_targets

    foreach pkg : packages
        pkg_pkgdir = '@0@.pkgdir'.format(pkg)
        resindex_cmd += [resources_dir / pkg_pkgdir]

        if transpile_glsl
            resindex_cmd += [meson.current_build_dir() / pkg_pkgdir]
        endif
    endforeach

    if transpile_glsl
        resindex_deps += essl_targets
        resindex_cmd += [
            '--exclude', '**/*.spv',
            '--exclude', '**/*.glslh',
        ]
    endif

    resindex_cmd += [
        '--exclude', '**/*.build',
        '--depfile', '@DEPFILE@',
    ]

    resindex = custom_target(
        command : resindex_cmd,
        depfile : 'res-index.inc.h.d',
        output : 'res-index.inc.h',
        depends : resindex_deps,
        build_by_default : false,
    )

    taisei_deps += declare_dependency(include_directories : include_directories('.'))

    meson.add_install_script(
        res_index_install_command, resindex, data_path,
        install_tag : res_install_tag,
    )

    subdir_done()
endif

# NOTE: the rest doesn't apply to emscripten/static-res-index, because of the subdir_done()

install_data(
    gamecontrollerdb_file,
    install_dir : data_path,
    install_tag : res_install_tag,
)

# Package or install all static pkgdirs
foreach pkg : packages
    pkg_pkgdir = '@0@.pkgdir'.format(pkg)
    pkg_zip = '@0@.zip'.format(pkg)
    pkg_path = join_paths(meson.current_source_dir(), pkg_pkgdir)

    if package_data
        bindist_deps += custom_target(pkg_zip,
            command : [pack_command,
                pkg_path,
                '@OUTPUT@',
                '--depfile', '@DEPFILE@',
                '--exclude', '**/meson.build',
            ],
            output : pkg_zip,
            depfile : '@0@.d'.format(pkg_zip),
            install : true,
            install_dir : data_path,
            install_tag : res_install_tag,
            console : true,
        )
    else
        glob_result = run_command(glob_command, pkg_path, '**/meson.build', check : true)
        install_subdir(
            pkg_pkgdir,
            install_dir : data_path,
            install_tag : res_install_tag,
            exclude_files : glob_result.stdout().split('\n')
        )
    endif
endforeach

# Package or install the special l10n pkgdir
if package_data
    l10n_pkg_zip = '@0@.zip'.format(l10n_pkg_name)
    bindist_deps += custom_target(l10n_pkg_zip,
        command : [pack_command,
            l10n_build_dir,
            '@OUTPUT@',
            '--prefix', l10n_subdir_name,
            '--exclude', '*.zip',
            '--depfile', '@DEPFILE@',
        ],
        output : l10n_pkg_zip,
        depends : l10n_mo_targets,
        depfile : '@0@.d'.format(l10n_pkg_zip),
        install : true,
        install_dir : data_path,
        install_tag : res_install_tag,
        console : true,
    )
else
    bindist_deps += l10n_mo_targets
endif

if host_machine.system() == 'nx'
    # Package shaders that were transpiled
    shader_pkg_zip = '01-es-shaders.zip'
    shader_pkg_path = join_paths(shaders_build_dir, '..')
    if package_data
        bindist_deps += custom_target(shader_pkg_zip,
            command : [pack_command,
                shader_pkg_path,
                '@OUTPUT@',
                '--depfile', '@DEPFILE@',
                '--exclude', '**/*.spv',
                '--exclude', '**/meson.build',
            ],
            output : shader_pkg_zip,
            depends : essl_targets,
            depfile : '@0@.d'.format(shader_pkg_zip),
            install : true,
            install_dir : data_path,
            install_tag : res_install_tag,
            console : true,
        )
    else
        glob_result = run_command(
            glob_command, shaders_build_dir, '**/*.spv', '**/meson.build',
            check : true
        )
        install_subdir(
            shaders_build_dir,
            install_dir : data_path,
            install_tag : res_install_tag,
            exclude_files : glob_result.stdout().split('\n')
        )
    endif
endif
