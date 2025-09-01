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

i18n.gettext(meson.project_name(), install_dir: data_path / '00-taisei.pkgdir' / 'i18n', args: xgettext_args)

quoted = []
foreach l : fs.read('LINGUAS').strip().split(' ')
    quoted += '"@0@"'.format(l)
endforeach

config.set(
    'TAISEI_BUILDCONF_LANGUAGES',
    '{@0@}'.format(', '.join(quoted))
)
