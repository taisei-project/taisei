
if install_docs
    rst2html5 = find_program(
        'rst2html5',
        'rst2html5.py',
        'rst2html5-docutils',
        'rst2html5-3',
        'rst2html53',
        'rst2html5-3.py',
        'rst2html4',
        'rst2html4.py',
        'rst2html4-3',
        'rst2html43',
        'rst2html4-3.py',
        'rst2html4-docutils',
        'rst2html',
        'rst2html.py',
        'rst2html-3',
        'rst2html3',
        'rst2html-3.py',
        'rst2html-docutils',
        required : false
    )

    if not rst2html5.found()
        error('rst2html5 from docutils is required to build documentation. Please install it, or disable the `docs` option if you don\'t want it.')
    endif

    rst_to_html = generator(rst2html5,
        arguments : [
            '--no-datestamp',
            '--no-source-link',
            '--smart-quotes=yes',
            '--no-compact-lists',
            '--stylesheet-path=minimal.css,plain.css,@0@/doc/taisei.css'.format(meson.source_root()),
            '@INPUT@',
            '@OUTPUT@',
        ],
        output : '@BASENAME@.html',
    )

    rst_docs = [
        'ENVIRON.rst',
        'GAME.rst',
        # 'ANIMATION_FORMAT.rst'
    ]

    txt_docs = [
        'README.txt',
        'STORY.txt',
    ]

    foreach f : txt_docs
        custom_target(f,
            command : [eolconv_command, host_eol_style, '@INPUT@', '@OUTPUT@'],
            input : f,
            output : f,
            install : true,
            install_dir : doc_path
        )
    endforeach

    foreach f : rst_docs
        name = f.split('.')[0] + '.html'
        custom_target(name,
            command : [eolconv_command, host_eol_style, '@INPUT@', '@OUTPUT@'],
            input : rst_to_html.process(f),
            output : name,
            install : true,
            install_dir : doc_path,
            depend_files : files('taisei.css')
        )
    endforeach

    if angle_enabled and host_machine.system() == 'windows'
        custom_target(f,
            command : [eolconv_command, host_eol_style, '@INPUT@', '@OUTPUT@'],
            input : 'LICENSE.ANGLE',
            output : 'LICENSE.txt',
            install : true,
            install_dir : join_paths(bindir, 'ANGLE')
        )
    endif
endif
