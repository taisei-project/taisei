
stages_src = []

stages = [
    'stage1',
    'stage2',
    'stage4',
    'stage5',
    'stage6',
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
    'extra.c',
)

if is_developer_build
    stages_src += files(
        'dpstest.c',
        'corotest.c',
    )
endif
