
resources_dir = meson.current_source_dir()

packages = [
    '00-taisei',
]

foreach pkg : packages
    pkg_pkgdir = '@0@.pkgdir'.format(pkg)
    pkg_zip = '@0@.zip'.format(pkg)
    pkg_path = join_paths(meson.current_source_dir(), pkg_pkgdir)

    subdir(pkg_pkgdir)

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
