#!/usr/bin/env python3

#
#   Install dylib dependencies, strip debugging symbols, and deal with the shitfucked digital abortion that is the macOS dynamic linker.
#

import re
import sys
import subprocess
import itertools
import shutil
from os import environ, pathsep
from pathlib import Path

build_root = Path(environ['MESON_BUILD_ROOT'])
source_root = Path(environ['MESON_SOURCE_ROOT'])
install_prefix = Path(environ['MESON_INSTALL_DESTDIR_PREFIX'])

args = sys.argv[1:]

macos_lib_paths = filter(None, (Path(x) for x in args.pop(0).split(pathsep)))
macos_tool_paths = filter(None, (Path(x) for x in args.pop(0).split(pathsep)))
macos_tool_prefix = args.pop(0)

if macos_tool_paths:
    environ['PATH'] = pathsep.join(itertools.chain((str(p) for p in macos_tool_paths), [environ['PATH']]))

exe_path = install_prefix / 'Taisei.app' / 'Contents' / 'MacOS' / 'Taisei'
dylib_dir_path = exe_path.parent / 'dylibs'
dylib_dir_path.mkdir(mode=0o755, parents=True, exist_ok=True)


def tool(name):
    def func(*args):
        cmd = [macos_tool_prefix + name] + list(args)
        print(' '.join(('{0!r}' if ' ' in str(s) else '{0}').format(str(s)) for s in cmd))
        return subprocess.check_output(cmd, universal_newlines=True)
    return func

otool = tool('otool')
install_name_tool = tool('install_name_tool')
strip = tool('strip')


def fix_libs(opath):
    handled = set()
    regex = re.compile(r'\s*(.*?\.dylib) \(')

    def install(src, dst):
        src = str(src)
        dst = str(dst)
        print('Installing {0} as {1}'.format(src, dst))
        shutil.copy(str(src), str(dst))

    def fix(path):
        for lib in regex.findall(otool('-L', path)):
            if lib.startswith('/usr/lib') or lib.startswith('@'):
                continue

            src_lib_path = Path(lib)
            dst_lib_path = dylib_dir_path / src_lib_path.name

            install_name_tool(path, '-change', lib, '@executable_path/dylibs/{0}'.format(dst_lib_path.name))

            if dst_lib_path in handled:
                continue

            install(src_lib_path, dst_lib_path)
            handled.add(dst_lib_path)
            fix(dst_lib_path)

        install_name_tool(path, '-id', path.name)
        strip('-S', path)

    fix(opath)
    return handled

new_files = fix_libs(exe_path)

if new_files:
    with (build_root / 'meson-logs' / 'install-log.txt').open('a') as f:
        f.write('# ...except when it does :^)\n')
        for i in new_files:
            f.write('{0}\n'.format(str(i)))
