format_strings = i18n.xgettext('format_strings', taisei_src, args : [ '-kF_', '--from-code=utf-8' ], install : false)

format_string_list = custom_target(
    'format_string_list.c',
    output : 'format_string_list.c',
    input : format_strings,
    command : [ extract_format_strings_script, '@INPUT@', '@OUTPUT@' ]
)

i18n_src = files(
    'i18n.c',
    'format_strings.c',
)

i18n_src += format_string_list
