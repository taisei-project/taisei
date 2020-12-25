
stages_src = []

stages = [
    'stage1',
    'stage2',
    'stage5',
    'stage6',
    'stagex',
]

foreach stage : stages
    subdir(stage)
    stages_src += get_variable('@0@_src'.format(stage))
endforeach

# TODO: reorganize all the remaining stages to use a separate directory and put
# them into the "stages" array

stages_src += files(
    'stage3.c',
    'stage3_events.c',
    'stage4.c',
    'stage4_events.c',
)

if is_developer_build
    stages_src += files(
        'dpstest.c',
        'corotest.c',
    )
endif
