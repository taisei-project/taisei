i18n = import('i18n')
fs = import('fs')

xgettext_args = [
'--from-code=utf-8',
'-kN_',
'-kMSG:2',
'-kT_NARRATOR',
'-kT_REIMU',
'-kT_MARISA',
'-kT_YOUMU',
'-kT_YUKARI',
'-kT_KANAKO',
'-kT_SUWAKO',
'-kT_SANAE',
'-kT_SUMIREKO',
'-kT_YUYUKO',
'-kT_ELLY',
'-kT_KURUMI',
'-kT_REMILIA',
'-kT_FLANDRE',
'-kT_PATCHOULI',
]

i18n_pkg_name = '10-i18n'

i18n_mo_targets = i18n.gettext(meson.project_name(),
    install : not package_data,
    install_dir : data_path / '@0@.pkgdir'.format(i18n_pkg_name) / 'i18n',
    args : xgettext_args,
)[0]

if package_data
    i18n_pkg_zip = '@0@.zip'.format(i18n_pkg_name)
    bindist_deps += custom_target(i18n_pkg_zip,
        command : [pack_command,
            meson.current_build_dir(),
            '@OUTPUT@',
            '--prefix', 'i18n',
            '--exclude', '*.zip',
            '--depfile', '@DEPFILE@',
        ],
        output : i18n_pkg_zip,
        depends : i18n_mo_targets,
        depfile : '@0@.d'.format(i18n_pkg_zip),
        install : true,
        install_dir : data_path,
        install_tag : res_install_tag,
        console : true,
    )
else
    bindist_deps += i18n_mo_targets
endif

quoted = []
foreach l : fs.read('LINGUAS').strip().split(' ')
    quoted += '"@0@"'.format(l)
endforeach

config.set(
    'TAISEI_BUILDCONF_LANGUAGES',
    '{@0@}'.format(', '.join(quoted))
)
