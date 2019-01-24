
subdir('shader')

dirs = ['bgm', 'gfx', 'models', 'sfx', 'shader', 'fonts']

if package_data
    archive = '00-taisei.zip'
    pack_exe = find_program('../scripts/pack.py')
    pack = custom_target('packed data files',
        command : [pack_exe,
            '@OUTPUT@',
            meson.current_source_dir(),
            '@DEPFILE@',
            dirs,
        ],
        output : archive,
        depfile : 'pack.d',
        install : true,
        install_dir : data_path,
        # depends : glsl_targets,
    )
else
    foreach dir : dirs
        install_subdir(dir, install_dir : data_path)
    endforeach
endif

resources_dir = meson.current_source_dir()
