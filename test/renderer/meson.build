
tests = [
    'triangle',
    'texture',
]

foreach test : tests
    executable(
        test, '@0@.c'.format(test),
        dependencies : libtaisei_dep,
        include_directories : test_incdir,
        install : false,
    )
endforeach
