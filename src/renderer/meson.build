
renderer_src = files(
    'api.c',
)

subdir('common')
renderer_src += r_common_src

renderer = get_option('renderer')

subdir(renderer)
renderer_src += get_variable('@0@_src'.format(renderer))
