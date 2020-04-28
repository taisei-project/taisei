
stages_src = files(
    'stages.c',
)

stages = [
    'stage1',
    'stage2',
    'stage3',
    'stage4',
    'stage5',
    'stage6',
    'stagex',
]

foreach stage : stages
    subdir(stage)
    stages_src += get_variable('@0@_src'.format(stage))
endforeach

if use_testing_stages
    stages_src += files(
        'dpstest.c',
    )
endif

stages_srcdir = meson.current_source_dir()
