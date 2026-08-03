
opt_tests = get_option('tests'
    ).disable_auto_if(not is_developer_build
    ).require(have_libtaisei, error_message : 'Can not build tests for this platform')

if opt_tests.disabled()
    subdir_done()
endif

test_incdir = include_directories('.')

subdir('renderer')
subdir('i18n')

tests = [
    { 'name' : 'shader_transpiler' },
    { 'name' : 'zip_io',
      'depends' : zip_targets,
      'args' : resources_build_dir },
]

foreach tdict : tests
    test(tdict['name'], executable(
            tdict['name'], '@0@.c'.format(tdict['name']),
            dependencies : libtaisei_dep,
            include_directories : test_incdir,
            install : false,
        ),
        depends : tdict.get('depends', []),
        args :  tdict.get('args', []),
    )
endforeach

if have_taisei_exe
    test('basic_replay', taisei,
        args : ['-R', files('test-replay.tsr')],
        env : dev_env)
endif
