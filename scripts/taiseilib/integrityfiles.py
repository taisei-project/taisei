
import os
import gnupg
import hashlib

from pathlib import Path

gpg = gnupg.GPG()

def check_for_key(gpg):
    if 'TAISEI_SIGN_KEY' in os.environ:
        # this can be an email address or a key ID
        sign_key = os.environ['TAISEI_SIGN_KEY']
        key_list = gpg.list_keys(keys=sign_key)
        if len(key_list) == 0:
            raise ValueError('TAISEI_SIGN_KEY: id specified was invalid or not found')
        return sign_key
    return None

def gen_integrity_files(archive):
    sign_key = check_for_key(gpg)

    with open(archive, 'rb') as file:
        gpg.sign_file(file, keyid = sign_key, detach=True, output='{0}.sig'.format(archive))
        file.seek(0) # reset the read position in memory so the file can be "read()" again
        sha256output = '{0}  {1}'.format(hashlib.sha256(file.read()).hexdigest(), archive.name)
        Path('{0}.sha256sum'.format(archive)).write_text(sha256output)

def verify_integrity_files(archive):
    sign_key = check_for_key(gpg)

    with open('{0}.sig'.format(archive), 'rb') as sig:
        verified = gpg.verify_file(sig, str(archive)) # str() necessary for Python3.6 on Windows builders
        if not verified:
            raise ValueError('could not verify gpg signature on archive: {0}'.format(archive))
        elif not sign_key is None and sign_key != verified.key_id:
            raise ValueError('could not verify gpg signature on archive: {0}, key mismatch'.format(archive))

    sha256input = Path('{0}.sha256sum'.format(archive)).read_text().strip()
    sha256output = '{0}  {1}'.format(hashlib.sha256(archive.read_bytes()).hexdigest(), archive.name)
    if str(sha256input) != str(sha256output):
        raise ValueError('could not verify sha256sum on archive: {0}, mismatch: {1} (input) != {2} (output)'.format(archive, sha256input, sha256output))

def main(args):
    import argparse

    parser = argparse.ArgumentParser(description = 'Generate integrity files for release.', prog = args[0])

    parser.add_argument('archive', type=Path, nargs='?',
        help='Path to the archive you want to generate integrity files for (i.e: /path/to/Taisei-vx.y-release.tar.xz)')

    args = parser.parse_args(args[1:])

    if args.archive is None:
        raise ValueError('archive path not specified to generate integrity files')

    gen_integrity_files(args.archive)
    verify_integrity_files(args.archive)
