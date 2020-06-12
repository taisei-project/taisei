#!/usr/bin/env python3

from pathlib import Path
import subprocess
import re
import tempfile
import shutil

print(Path.cwd())

SAMPLE_RATE = 48000

with tempfile.TemporaryDirectory() as temp:
    cwd = Path.cwd()
    temp = Path(temp)

    for p in cwd.glob('**/*.bgm'):
        raw = p.read_text()
        print(p)

        meta = {}
        tags = re.findall('(.*?) = (.*)', raw)
        tagmap = {
            'title': 'TITLE',
            'artist': 'ARTIST',
            'comment': 'comment',
            'loop_point': 'LOOPSTART',
        }

        order = ['TITLE', 'ARTIST', 'ALBUM', 'comment', 'LOOPSTART', 'LOOPEND']

        loopfile = None

        for tag, value in tags:
            value = value.replace('\\n', '\n')
            if tag == 'loop':
                loopfile = value
            else:
                if tag == 'loop_point':
                    value = str(int(float(value) * SAMPLE_RATE))

                meta[tagmap[tag]] = value

        meta['ALBUM'] = 'Taisei Project'

        assert loopfile is not None
        assert loopfile.startswith('res/bgm/')
        basename = loopfile[len('res/bgm/'):]
        loopfile = cwd / basename
        assert loopfile.is_file()

        if 'LOOPSTART' in meta:
            p = subprocess.run(['soxi', '-s', loopfile], capture_output=True, check=True)
            meta['LOOPEND'] = p.stdout.strip().decode('utf8')

        tmpfile = temp / basename
        tmpfile.parent.mkdir(parents=True, exist_ok=True)

        ffmpeg_args = ['ffmpeg', '-i', loopfile]

        for tag, value in sorted(meta.items(), key=lambda pair: order.index(pair[0])):
            ffmpeg_args += ['-metadata', f'{tag}={value}']

        ffmpeg_args += ['-codec', 'copy', tmpfile]

        print(ffmpeg_args)
        subprocess.run(ffmpeg_args, check=True)

        shutil.move(tmpfile, loopfile)
