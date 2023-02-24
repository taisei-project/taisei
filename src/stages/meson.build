
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
]

foreach stage : stages
    subdir(stage)
    stages_src += get_variable('@0@_src'.format(stage))
endforeach

stages_src += files(
    'extra.c',  # stub
)

if use_testing_stages
    stages_src += files(
        'dpstest.c',
        'corotest.c',
    )
endif

stages_srcdir = meson.current_source_dir()
