
resources_dir = meson.current_source_dir()

packages = [
    '00-taisei',
]

foreach pkg : packages
    pkg_pkgdir = '@0@.pkgdir'.format(pkg)
    pkg_zip = '@0@.zip'.format(pkg)
    pkg_path = join_paths(meson.current_source_dir(), pkg_pkgdir)

    subdir(pkg_pkgdir)

    if host_machine.system() == 'emscripten'
        continue
    endif

    if package_data
        custom_target(pkg_zip,
            command : [pack_command,
                pkg_path,
                '@OUTPUT@',
                '--depfile', '@DEPFILE@',
            ],
            output : pkg_zip,
            depfile : '@0@.d'.format(pkg_zip),
            install : true,
            install_dir : data_path,
        )
    else
        glob_result = run_command(glob_command, pkg_path, '**/meson.build')
        assert(glob_result.returncode() == 0, 'Glob script failed')
        install_subdir(pkg_pkgdir, install_dir : data_path, exclude_files : glob_result.stdout().split('\n'))
    endif
endforeach

if host_machine.system() == 'emscripten'
    emfs_basedir = 'emscripten-fs'
    emfs_target_dir = join_paths(meson.current_build_dir(), emfs_basedir)
    emfs_shader_dir = shaders_build_dir

    # TODO: maybe just generate a huge list of --preload-file arguments instead
    # of constructing a whole tree. This code was originally meant to implement
    # a different preload-less approach that didn't quite work out.

    em_filesystem = custom_target(emfs_basedir,
        command : [
            gen_emscripten_fs_command,
            emfs_target_dir,
            emfs_shader_dir,
            '--depfile', '@DEPFILE@',
            '--stupid-useless-file', '@OUTPUT@',
        ],
        depends : essl_targets,
        output : '@0@.suf'.format(emfs_basedir),
        depfile : '@0@.d'.format(emfs_basedir),
    )
endif
