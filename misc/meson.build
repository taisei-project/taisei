
if macos_app_bundle
    install_data(join_paths('icons', 'Taisei.icns'), install_dir : datadir)

    plist = custom_target('macOS property list',
        command : [preprocess_command, '@INPUT@', '@OUTPUT@'],
        build_by_default : true,
        build_always_stale : true,
        input : 'Info.plist.in',
        output : 'Info.plist',
        install : true,
        install_dir : join_paths(bundle_dir, 'Contents'),
        install_tag : 'runtime',
    )

    version_deps += plist
    bindist_deps += plist
endif

if angle_enabled and host_machine.system() == 'windows'
    angle_launcher = 'taisei-angle.bat'
    angle_bat = custom_target(angle_launcher,
        command : [eolconv_command, host_eol_style, '--no-bom', '@INPUT@', '@OUTPUT@'],
        input : angle_launcher,
        output : angle_launcher,
        install : true,
        install_dir : bindir,
        install_tag : 'runtime',
    )
    bindist_deps += angle_bat
endif

glslc_test_basic_file = files('glslc-test-basic.frag.glsl')
glslc_test_uniform_location_file = files('glslc-test-uniform-location.frag.glsl')
