
if get_option('docs')
    pandoc = find_program('pandoc', required : false)

    if not pandoc.found()
        error('pandoc is required to build documentation. Please install it, or disable the `docs` option if you don\'t want it.')
    endif

    md_to_html = generator(pandoc,
        arguments: ['--from', 'markdown_github', '--to', 'html', '--standalone', '@INPUT@', '--output', '@OUTPUT@'],
        output: '@BASENAME@.html',
    )

    md_docs = [
        'ENVIRON.md',
        'GAME.md',
    ]

    txt_docs = [
        'README.txt',
        'STORY.txt',
    ]

    eol = host_machine.system() == 'windows' ? 'crlf' : 'lf'

    foreach f : txt_docs
        custom_target(f,
            command : [eolconv, eol, '@INPUT@', '@OUTPUT@'],
            input : f,
            output : f,
            install : true,
            install_dir : doc_path
        )
    endforeach

    foreach f : md_docs
        name = f.split('.')[0] + '.html'
        custom_target(name,
            command : [eolconv, eol, '@INPUT@', '@OUTPUT@'],
            input : md_to_html.process(f),
            output : name,
            install : true,
            install_dir : doc_path
        )
    endforeach
endif
