
stages_src = []

stages = [
    'stagex'
]

foreach stage : stages
    subdir(stage)
    stages_src += get_variable('@0@_src'.format(stage))
endforeach

# TODO: reorganize all the remaining stages to use a separate directory and put
# them into the "stages" array

stages_src += files(
    'stage1.c',
    'stage1_events.c',
    'stage2.c',
    'stage2_events.c',
    'stage3.c',
    'stage3_events.c',
    'stage4.c',
    'stage4_events.c',
    'stage5.c',
    'stage5_events.c',
    'stage6.c',
    'stage6_events.c',
)

if is_developer_build
    stages_src += files(
        'dpstest.c',
    )
endif

stages_src += files('corotest.c')
