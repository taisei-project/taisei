
if get_option('docs')
    pandoc = find_program('pandoc', required : false)

    if not pandoc.found()
        error('pandoc is required to build documentation. Please install it, or disable the `docs` option if you don\'t want it.')
    endif

    r = run_command(pandoc, '--list-input-formats')

    if r.returncode() != 0
        error('Couldn\'t retrieve list of input formats supported by pandoc (error code @0@)\n\n@1@'.format(
            r.returncode(),
            r.stderr()
        ))
    endif

    formats = r.stdout().split()
    input_format = 'ghc'
    input_format_fallback = 'markdown_github'

    if not formats.contains(input_format)
        message('pandoc doesn\'t support the \'@0@\' format, falling back to @1@'.format(
            input_format,
            input_format_fallback
        ))

        if not formats.contains(input_format_fallback)
            error('pandoc supports neither \'@0@\' nor \'@1@\''.format(
                input_format,
                input_format_fallback
            ))
        endif

        input_format = input_format_fallback
    endif

    md_to_html = generator(pandoc,
        arguments: [
            '--from', input_format,
            '--to', 'html5',
            '--standalone',
            '--self-contained',
            '--table-of-contents',
            '@EXTRA_ARGS@',
            '@INPUT@',
            '--output', '@OUTPUT@',
        ],
        output: '@BASENAME@.html',
    )

    md_docs = [
        ['ENVIRON.md', 'Environment Variables'],
        ['GAME.md', 'Game Manual'],
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

    foreach pair : md_docs
        f = pair[0]
        title = pair[1]

        name = f.split('.')[0] + '.html'
        custom_target(name,
            command : [eolconv, eol, '@INPUT@', '@OUTPUT@'],
            input : md_to_html.process(f, extra_args : [
                '--metadata=title=@0@ - Taisei Project'.format(title)
            ]),
            output : name,
            install : true,
            install_dir : doc_path
        )
    endforeach
endif
