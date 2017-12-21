
from .common import (
    meson_introspect,
    ninja,
)

from pathlib import Path
from contextlib import contextmanager, suppress
from tempfile import TemporaryDirectory

import os


def get_build_opts(build_dir):
    opts = meson_introspect('--buildoptions', cwd=build_dir)
    return {opt['name']: opt for opt in opts}


@contextmanager
def temp_install(build_dir):
    with TemporaryDirectory(prefix='taisei-tempinstall') as tempdir:
        log_path = Path(build_dir) / 'meson-logs' / 'install-log.txt'
        log_bak_path = log_path.with_name(log_path.name + '.bak')

        prefix = get_build_opts(build_dir)['prefix']['value']
        install_path = Path(tempdir + prefix)

        env = os.environ.copy()
        env['DESTDIR'] = tempdir

        with suppress(FileNotFoundError):
            log_path.rename(log_bak_path)

        try:
            for target in ('install',): # ('reconfigure', 'install',):
                ninja(target, cwd=build_dir, env=env)

            yield install_path
        finally:
            with suppress(FileNotFoundError):
                log_bak_path.rename(log_path)
