i18n = import('i18n')


xgettext_args = [
'--from-code=utf-8',
'-kN_',
'-kTITLE:2,3',
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

i18n.gettext(config.get_unquoted('TAISEI_BUILDCONF_GETTEXT_PACKAGE'), install_dir: config.get_unquoted('TAISEI_BUILDCONF_LOCALE_PATH'), args: xgettext_args)

