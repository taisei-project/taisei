
resources_dir = meson.current_source_dir()
resources_build_dir = meson.current_build_dir()

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

zip_sha256 = package_data
zip_install_dir = data_path

if zip_sha256
    zip_install_dir = [zip_install_dir, false]
endif

zips = []

# Package or install all static pkgdirs
foreach pkg : packages
    pkg_pkgdir = '@0@.pkgdir'.format(pkg)
    pkg_zip = '@0@.zip'.format(pkg)
    pkg_path = join_paths(meson.current_source_dir(), pkg_pkgdir)

    if package_data
        zips += {
            'zip' : pkg_zip,
            'source' : pkg_path,
            'pack_args' : ['--exclude', '**/meson.build']
        }
    else
        glob_result = run_command(glob_command, pkg_path, '**/meson.build', check : true)
        install_subdir(
            pkg_pkgdir,
            install_dir : data_path,
            install_tag : res_install_tag,
            exclude_files : glob_result.stdout().strip().split('\n')
        )
    endif
endforeach

# Package or install the special l10n pkgdir
if package_data
    zips += {
        'zip' : '@0@.zip'.format(l10n_pkg_name),
        'source' : l10n_build_dir,
        'pack_args' : [
            '--prefix', l10n_subdir_name,
            '--exclude', '*.zip',
        ],
        'depends' : l10n_mo_targets,
    }
else
    bindist_deps += l10n_mo_targets
endif

if host_machine.system() == 'nx'
    # Package shaders that were transpiled
    shader_pkg_zip = '01-es-shaders.zip'
    shader_pkg_path = join_paths(shaders_build_dir, '..')
    if package_data
        zips += {
            'zip' : shader_pkg_zip,
            'source' : shader_pkg_path,
            'pack_args' : [
                '--exclude', '**/*.spv',
                '--exclude', '**/meson.build',
            ],
            'depends' : essl_targets,
        }
    else
        glob_result = run_command(
            glob_command, shaders_build_dir, '**/*.spv', '**/meson.build',
            check : true
        )
        install_subdir(
            shaders_build_dir,
            install_dir : data_path,
            install_tag : res_install_tag,
            exclude_files : glob_result.stdout().strip().split('\n')
        )
    endif
endif

zip_targets = []

foreach zip : zips
    outputs = [zip['zip']]
    command = [pack_command,
        zip['source'],
        '@OUTPUT0@',
        '--depfile', '@DEPFILE@',
    ] + zip.get('pack_args', [])

    if zip_sha256
        command += ['--sha256sums', '@OUTPUT1@']
        outputs += ['@0@.sha256'.format(outputs[0])]
    endif

    zip_targets += custom_target(zip['zip'],
        command : command,
        output : outputs,
        depends : zip.get('depends', []),
        depfile : '@0@.d'.format(zip['zip']),
        install : true,
        install_dir : zip_install_dir,
        install_tag : res_install_tag,
        console : true,
    )
endforeach

bindist_deps += zip_targets
