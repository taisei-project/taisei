dirs = ['bgm', 'gfx', 'models', 'sfx', 'shader']

if taisei_deps.contains(dep_zip)
    archive = '00-taisei.zip'
    pack_exe = find_program('../scripts/pack.py')
    pack = custom_target('packed data files',
                        command : [pack_exe, '@OUTPUT@',
                                   meson.current_source_dir(),
                                   '@DEPFILE@',
                                   '@INPUT@'],
                        input : dirs,
                        output : archive,
                        depfile : 'pack.d',
                        install : true,
                        install_dir : data_path)
else
    foreach dir : dirs
        install_subdir(dir, install_dir : data_path)
    endforeach
endif
