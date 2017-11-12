doc_files = [
    'ENVIRON.md',
    'GAME.md',
    'README.txt',
    'STORY.txt',
]

if host_machine.system() == 'windows'
    foreach f : doc_files
        custom_target(f, command : [unix2dos, '@INPUT@', '@OUTPUT@'],
                      input : f, output : f.strip('../').split('.')[0] + '.txt',
                      install : true, install_dir : doc_path)
    endforeach
else
    install_data(doc_files, install_dir : doc_path)
endif
