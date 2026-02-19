
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
    'shader_transpiler',
]

foreach testname : tests
    e = executable(
        testname, '@0@.c'.format(testname),
        dependencies : libtaisei_dep,
        include_directories : test_incdir,
        install : false,
    )
    test(testname, e)
endforeach

