
if angle_enabled
    bindist_deps += custom_target('LICENSE.ANGLE',
        command : [eolconv_command, host_eol_style, '@INPUT@', '@OUTPUT@'],
        input : 'LICENSE.ANGLE',
        output : 'LICENSE.txt',
        install : true,
        install_dir : angle_install_path
    )
endif

rst2html_variants = []

# This is *completely* ridiculous, but apparently this thing has THAT many faces.
basenames = ['rst2html5', 'rst2html', 'rst2html4']
suffixes = ['', '-3', '3', '-docutils']

foreach basename : basenames
    foreach suffix : suffixes
        variant = '@0@@1@'.format(basename, suffix)
        rst2html_variants += [variant, '@0@.py'.format(variant)]
    endforeach
endforeach

rst2html5 = find_program(
    rst2html_variants,
    required : get_option('docs')
)

if rst2html5.found()
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
        bindist_deps += custom_target(f,
            command : [eolconv_command, host_eol_style, '@INPUT@', '@OUTPUT@'],
            input : f,
            output : f,
            install : true,
            install_dir : doc_path
        )
    endforeach

    foreach f : rst_docs
        name = f.split('.')[0] + '.html'
        bindist_deps += custom_target(name,
            command : [eolconv_command, host_eol_style, '@INPUT@', '@OUTPUT@'],
            input : rst_to_html.process(f),
            output : name,
            install : true,
            install_dir : doc_path,
            depend_files : files('taisei.css')
        )
    endforeach
endif
