
em_preamble = files('preamble.js')
em_postamble = files('postamble.js')
em_shell = files('shell.html')

if host_machine.system() == 'emscripten'
    install_data(
        files(
            'background.webp',
            'scythe.webp',
            'favicon.ico',
        ),
        install_dir : bindir,
        install_tag : 'runtime',
    )
    install_data(
        em_shell,
        rename : 'taisei.html',
        install_dir : bindir,
        install_tag : 'runtime',
    )
endif
