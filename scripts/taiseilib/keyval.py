
import os
import re
import string

from .common import TaiseiError

KV_REGEX = re.compile(r'^\s*(.*?)\s*=\s*(.*?)\s*$')


"""
Very dumb configuration parser compatible with Taisei's kvparser.c
"""


class KeyvalSyntaxError(TaiseiError):
    pass


def parse(fobj, filename='<unknown>', dest=None, missing_ok=False):
    if dest is None:
        dest = {}

    if isinstance(fobj, (str, os.PathLike)):
        try:
            with fobj.open('r') as real_fobj:
                return parse(real_fobj, filename=fobj, dest=dest)
        except FileNotFoundError:
            if missing_ok:
                return dest
            raise

    lnum = 0

    for line in fobj.readlines():
        lnum += 1
        line = line.lstrip()

        if not line or line.startswith('#'):
            continue

        m = KV_REGEX.match(line)

        if not m:
            raise KeyvalSyntaxError('{!s}:{}'.format(filename, lnum))

        key, val = m.groups()
        dest[key] = val

    return dest


def dump(src):
    lines = []

    for rkey, rval in src.items():
        key, val = str(rkey), str(rval)
        line = '{} = {}'.format(key, val)

        ok = True
        m = KV_REGEX.match(line)
        if not m:
            ok = False
        else:
            nkey, nval = m.groups()
            if key != nkey or val != nval:
                ok = False

        if not ok:
            raise KeyvalSyntaxError('Pair {!r} can\'t be represented'.format((rkey, rval)))

        lines.append(line)

    return '\n'.join(lines)


def strbool(s):
    try:
        return bool(int(s))
    except ValueError:
        pass

    s = str(s).lower()

    if s in ('on', 'yes', 'true'):
        return True

    if s in ('off', 'no', 'false'):
        return False

    raise TaiseiError('Invalid boolean value: {}'.format(s))
