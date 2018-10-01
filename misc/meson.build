
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

if angle_enabled and host_machine.system() == 'windows'
    angle_launcher = 'taisei-angle.bat'
    custom_target(angle_launcher,
        command : [eolconv, host_eol_style, '--no-bom', '@INPUT@', '@OUTPUT@'],
        input : angle_launcher,
        output : angle_launcher,
        install : true,
        install_dir : bindir,
    )
endif

glslc_test_file = files('glslc-test.frag.glsl')
