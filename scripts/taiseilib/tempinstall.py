
from .common import (
    meson_introspect,
    meson,
)

from pathlib import Path
from contextlib import contextmanager, suppress
from tempfile import TemporaryDirectory


def get_build_opts(build_dir):
    opts = meson_introspect('--buildoptions', cwd=str(build_dir))
    return {opt['name']: opt for opt in opts}


@contextmanager
def temp_install(build_dir, skip_subprojects=True):
    with TemporaryDirectory(prefix='taisei-tempinstall') as tempdir:
        log_path = Path(build_dir) / 'meson-logs' / 'install-log.txt'
        log_bak_path = log_path.with_name(log_path.name + '.bak')

        prefix = get_build_opts(build_dir)['prefix']['value']
        install_path = Path(tempdir + prefix)

        with suppress(FileNotFoundError):
            log_path.rename(log_bak_path)

        meson_cmd = ['install', '--destdir', tempdir]

        if skip_subprojects:
            meson_cmd += ['--skip-subprojects']

        try:
            meson(*meson_cmd, cwd=str(build_dir))
            yield install_path
        finally:
            with suppress(FileNotFoundError):
                log_bak_path.rename(log_path)
