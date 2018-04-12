#!/usr/bin/env python3

import pathlib
import re

api_regex = re.compile(r'^CGLM_EXPORT\s*.*?\s*glmc_([a-zA-Z_0-9]*)', re.DOTALL | re.MULTILINE)
inc_path = pathlib.Path(__file__).parent / 'include' / 'cglm'

with (inc_path / 'glm2call.h').open('w') as out:
    print('\n// Auto-generated, do not modify.', file=out)

    for path in (inc_path / 'call').glob('**/*.h'):
        print('\n// {0}'.format(path.relative_to(inc_path)), file=out)
        for name in api_regex.findall(path.read_text()):
            print('#undef glm_{0}\n#define glm_{0} glmc_{0}'.format(name), file=out)
