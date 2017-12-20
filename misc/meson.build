install_data(join_paths('gamecontrollerdb', 'gamecontrollerdb.txt'), install_dir : data_path)

if macos_app_bundle
    install_data(join_paths('icons', 'Taisei.icns'), install_dir : datadir)

    version_deps += custom_target('macOS property list',
        command : [preprocess_command, '@INPUT@', '@OUTPUT@'],
        build_always : true,
        input : 'Info.plist.in',
        output : 'Info.plist',
        install : true,
        install_dir : join_paths(bundle_dir, 'Contents')
    )
endif
