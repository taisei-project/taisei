
import os
import gnupg
import hashlib

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
        sha256digest = hashlib.sha256(file.read()).hexdigest()
        sha256output = '{0}  {1}'.format(sha256digest, archive)
        file.close()

    with open('{0}.sha256sum'.format(archive), 'w') as sha256file:
        print(sha256output, file = sha256file)
        sha256file.close()

def verify_integrity_files(archive):
    sign_key = check_for_key(gpg)

    with open('{0}.sig'.format(archive), 'rb') as sig:
        verified = gpg.verify_file(sig, archive)
        if not verified:
            raise ValueError('could not verify gpg signature on archive: {0}'.format(archive))
        elif not sign_key is None and sign_key != verified.key_id:
            raise ValueError('could not verify gpg signature on archive: {0}, key mismatch'.format(archive))

        sig.close()
    with open(archive, 'rb') as file:
        with open('{0}.sha256sum'.format(archive), 'r') as sha256file:
            sha256input = sha256file.readline().strip()
            sha256output = '{0}  {1}'.format(hashlib.sha256(file.read()).hexdigest(), archive)
            if str(sha256input) != str(sha256output):
                raise ValueError('could not verify sha256sum on archive: {0}, mismatch: {1} != {2}'.format(archive, sha256input, sha256output))
            sha256file.close()
        file.close()

def main(args):
    import argparse

    parser = argparse.ArgumentParser(description = 'Generate integrity files for release.', prog = args[0])

    parser.add_argument('archive', type=str, nargs='?',
        help='Path to the archive you want to generate integrity files for (i.e: /path/to/Taisei-vx.y-release.tar.xz)')

    args = parser.parse_args(args[1:])

    if args.archive is None:
        raise ValueError('archive path not specified to generate integrity files')

    gen_integrity_files(args.archive)
    verify_integrity_files(args.archive)
