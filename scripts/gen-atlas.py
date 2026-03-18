#!/usr/bin/env python3

import argparse
import dataclasses
import enum
import functools
import itertools
import math
import random
import re
import rectpack
import shutil
import struct
import subprocess
import sys

from concurrent.futures import ProcessPoolExecutor, ThreadPoolExecutor, as_completed
from contextlib import ExitStack
from pathlib import Path
from tempfile import TemporaryDirectory
from typing import Optional

try:
    import compression.zstd as zstd
except ImportError:
    import backports.zstd as zstd

from PIL import Image

from taiseilib.common import TaiseiError, run_main, update_text_file, wait_for_futures
import taiseilib.atlas as atlas
import taiseilib.keyval as keyval


texture_formats = [s[1:] for s in atlas.SPRITE_SUFFIXES]
TSATLAS_FORMAT  = 'tsatlas'


# ---------------------------------------------------------------------------
# Utilities
# ---------------------------------------------------------------------------

def gil_enabled():
    if hasattr(sys, '_is_gil_enabled'):
        return sys._is_gil_enabled()
    return True


def ceildiv(x, y):
    return x // y + int(x % y != 0)


def align_size(size, alignment=4):
    return tuple(ceildiv(x, alignment) * alignment for x in size)


def get_bin_occupied_bounds(bin):
    xmin = ymin = 0xffffffffffffffff
    xmax = ymax = 0

    for rect in bin:
        xmin = min(xmin, rect.left)
        xmax = max(xmax, rect.right)
        ymin = min(ymin, rect.bottom)
        ymax = max(ymax, rect.top)

    return rectpack.geometry.Rectangle(xmin, ymin, *align_size((xmax - xmin, ymax - ymin)))


def sprite_sources_iter(srcroot):
    def imgfilter(path):
        return (
            path.is_file()
            and path.suffix.lower() in atlas.SPRITE_SUFFIXES
            and path.with_suffix('').suffix != '.alphamap'
        )
    for path in sorted(filter(imgfilter, srcroot.glob('**/*.*'))):
        yield path


# ---------------------------------------------------------------------------
# Rect packing
# ---------------------------------------------------------------------------

@functools.total_ordering
class PackResult:
    def __init__(self, packer, rects):
        self.bins = list(packer)
        self.num_images_packed = sum(map(len, self.bins))
        self.success = self.num_images_packed == len(rects)
        assert self.num_images_packed <= len(rects)

        self._bin_bounds  = [get_bin_occupied_bounds(b) for b in self.bins]
        self.total_area   = sum(b.area() for b in self._bin_bounds)
        self.longest_side = functools.reduce(
            max, itertools.chain(*((r.width, r.height) for r in self._bin_bounds)), 0)

    @property
    def num_bins(self):
        return len(self.bins)

    def __repr__(self):
        return (f'PackResult(bins={self._bin_bounds}, '
                f'num_images_packed={self.num_images_packed}, '
                f'total_area={self.total_area}, success={self.success})')

    def __eq__(self, other):
        return (self.num_bins          == other.num_bins
            and self.num_images_packed == other.num_images_packed
            and self.total_area        == other.total_area
            and self.longest_side      == other.longest_side)

    def __lt__(self, other):  # less == better
        if self.num_images_packed != other.num_images_packed:
            return self.num_images_packed > other.num_images_packed
        if self.num_bins != other.num_bins:
            return self.num_bins < other.num_bins
        if self.total_area != other.total_area:
            return self.total_area < other.total_area
        return self.longest_side < other.longest_side


def _bruteforcer_pack_one(rects, params, bin_size, single_bin):
    algo_name, (sort_name, sort_label) = params
    algo  = getattr(rectpack, algo_name)
    sort  = getattr(rectpack, sort_name)

    packer = rectpack.newPacker(
        rotation=False,
        pack_algo=algo,
        sort_algo=sort,
        bin_algo=rectpack.PackingBin.BFF,
    )

    if single_bin:
        packer.add_bin(*bin_size)
        for rect in rects:
            packer.add_rect(*rect)
    else:
        for rect in rects:
            packer.add_bin(*bin_size)
            packer.add_rect(*rect)

    packer.pack()
    return PackResult(packer, rects), algo_name, sort_label


def _bruteforcer_pack_chunk(rects, jobs):
    best_result, best_algo, best_sort = _bruteforcer_pack_one(rects, *jobs[0])

    for job in jobs[1:]:
        result, algo, sort = _bruteforcer_pack_one(rects, *job)
        if result < best_result:
            best_result, best_algo, best_sort = result, algo, sort

    return best_result, best_algo, best_sort


@dataclasses.dataclass
class PackSettings:
    bin_size: tuple[int, int]         # (width, height)
    bin_max_size: tuple[int, int]     # ignored if sweep_samples is 0
    sweep_samples: int
    single_bin: bool


def pack_rects_brute_force(executor, atlasname, rects, settings: PackSettings):
    algos_maxrects = [
        'MaxRectsBaf',
        'MaxRectsBl',
        'MaxRectsBlsf',
        'MaxRectsBssf',
    ]

    algos_guillotine = [
        'GuillotineBafLas',
        'GuillotineBafLlas',
        'GuillotineBafMaxas',
        'GuillotineBafMinas',
        'GuillotineBafSas',
        'GuillotineBafSlas',
        'GuillotineBlsfLas',
        'GuillotineBlsfLlas',
        'GuillotineBlsfMaxas',
        'GuillotineBlsfMinas',
        'GuillotineBlsfSas',
        'GuillotineBlsfSlas',
        'GuillotineBssfLas',
        'GuillotineBssfLlas',
        'GuillotineBssfMaxas',
        'GuillotineBssfMinas',
        'GuillotineBssfSas',
        'GuillotineBssfSlas',
    ]

    algos_skyline = [
        'SkylineBl',
        'SkylineBlWm',
        'SkylineMwf',
        'SkylineMwfWm',
        'SkylineMwfl',
        'SkylineMwflWm',
    ]

    sorts = [
        ('SORT_AREA',  'area'),
        ('SORT_LSIDE', 'longest-side'),
        ('SORT_PERI',  'perimeter'),
        ('SORT_SSIDE', 'shortest-side'),

        # These are not very useful
        # ('SORT_DIFF',  'difference'),
        # ('SORT_NONE',  'no'),
        # ('SORT_RATIO', 'ratio'),
    ]

    algos = []
    algos += algos_maxrects
    algos += algos_skyline
    algos += algos_guillotine

    variants = tuple(itertools.product(algos, sorts))

    num_samples = settings.sweep_samples

    min_area = max_w = max_h = 0
    min_w = min_h = 0

    for w, h, _ in rects:
        max_w   += w
        max_h   += h
        min_w    = max(min_w, w)
        min_h    = max(min_h, h)
        min_area += w * h

    max_w = min(settings.bin_max_size[0], max_w)
    max_h = min(settings.bin_max_size[1], max_h)
    min_w = math.ceil(max(min_w, min_area / max_h))
    min_h = math.ceil(max(min_h, min_area / max_w))

    def points(low, high):
        return set(round(low + (high - low) * i / (num_samples - 1)) for i in range(num_samples))

    jobs = []

    if num_samples > 0:
        for variant in variants:
            for w in points(min_w, max_w):
                jobs.append((variant, (w, settings.bin_max_size[1]), settings.single_bin))
            for h in points(min_h, max_h):
                jobs.append((variant, (settings.bin_max_size[1], h), settings.single_bin))
    else:
        for variant in variants:
            jobs.append((variant, settings.bin_size, settings.single_bin))

    random.shuffle(jobs)
    chunk_size = math.ceil(len(jobs) / executor._max_workers)

    prefix = '%12s |' % atlasname
    def log(*msg):
        print(prefix, *msg)

    futures = []
    for i in range(0, len(jobs), chunk_size):
        chunk = jobs[i:i + chunk_size]
        futures.append(executor.submit(_bruteforcer_pack_chunk, rects, tuple(chunk)))
        log("Submitted job #%d with %d samples" % (len(futures), len(chunk)))

    best: Optional[PackResult] = None
    best_algos: Optional[tuple[str, str]] = None

    for future in as_completed(futures):
        futures.remove(future)
        result, algo, sort = future.result()
        if best is None or result < best:
            log(' * {:>20} with {:20} | {!s}'.format(algo, sort + ' sort', result))
            best       = result
            best_algos = (algo, sort)

    assert best is not None
    assert best_algos is not None

    log('*' * 64)
    log(f'WINNER: {best_algos[0]} with {best_algos[1]} sort')
    log(f'\tBest result: {best}')
    log('*' * 64)

    return best


# ---------------------------------------------------------------------------
# Work plan
# ---------------------------------------------------------------------------

@dataclasses.dataclass
class PlannedBin:
    texture_id:  str
    actual_size: tuple   # (width, height) in pixels
    sprites:     list    # list[atlas.Sprite]; texture_region in pixel coords (not normalised)


@dataclasses.dataclass
class AtlasWorkPlan:
    atlasname: str
    bins:      list      # list[PlannedBin]


def build_work_plan(pack_result, sprites, atlasname, crop, border):
    def get_border(sprite):
        return max(border, int(sprite.config.get('border', border)))

    planned_bins = []

    for i, bin in enumerate(pack_result.bins):
        texture_id = atlasname if len(pack_result.bins) == 1 else f'{atlasname}_{i}'

        if crop:
            bounds      = get_bin_occupied_bounds(bin)
            actual_size = (bounds.width, bounds.height)
            offset      = (-bounds.x, -bounds.y)
        else:
            actual_size = (bin.width, bin.height)
            offset      = (0, 0)

        planned_sprites = []
        for rect in bin:
            sprite = sprites[rect.rid]
            b      = get_border(sprite)
            adj_x  = rect.x + offset[0]
            adj_y  = rect.y + offset[1]
            sprite.texture_id     = texture_id
            sprite.texture_region = atlas.Geometry(
                rect.width  - b * 2,
                rect.height - b * 2,
                adj_x + b,
                adj_y + b,
            )
            planned_sprites.append(sprite)

        planned_bins.append(PlannedBin(
            texture_id  = texture_id,
            actual_size = actual_size,
            sprites     = planned_sprites,
        ))

    return AtlasWorkPlan(atlasname=atlasname, bins=planned_bins)


# ---------------------------------------------------------------------------
# Sprite sort strategies for .tsatlas format
# ---------------------------------------------------------------------------

def sort_sprites_by_name(sprites):
    sprites.sort(key=lambda s: s.name)

def sort_sprites_by_perimeter(sprites):
    sprites.sort(key=lambda s: s.texture_region.width + s.texture_region.height)

def sort_sprites_by_area(sprites):
    sprites.sort(key=lambda s: s.texture_region.width * s.texture_region.height)


TSATLAS_SORT_STRATEGIES = {
    'none':      (None,                      'No sorting; keep rect-pack order'),
    'name':      (sort_sprites_by_name,      'Alphabetical by sprite name'),
    'perimeter': (sort_sprites_by_perimeter, 'Ascending perimeter (width + height)'),
    'area':      (sort_sprites_by_area,      'Ascending area (width × height)'),
}

TSATLAS_SORT_DEFAULT = 'name'


# ---------------------------------------------------------------------------
# Legacy mode: pack into texture images with sidecar files
# ---------------------------------------------------------------------------

def write_sprite_def(dst, data):
    dst.parent.mkdir(exist_ok=True, parents=True)
    update_text_file(dst, '# Autogenerated by the atlas packer, do not modify\n\n' + keyval.dump(data))


def write_texture_def(dst, texture, texture_fmt, global_overrides=None, local_overrides=None, alphamap_tex_fmt=None):
    dst.parent.mkdir(exist_ok=True, parents=True)
    data = {'source': f'res/gfx/{texture}.{texture_fmt}'}
    if alphamap_tex_fmt is not None:
        data['alphamap'] = f'res/gfx/{texture}.alphamap.{alphamap_tex_fmt}'
    if global_overrides:
        data.update(global_overrides)
    if local_overrides:
        data.update(local_overrides)
    update_text_file(dst, '# Autogenerated by the atlas packer, do not modify\n\n' + keyval.dump(data))


def execute_work_plan_image_files(executor, plan, dst, temp_dst, tex_format,
                                  texture_global_overrides, texture_local_overrides):
    futures = []

    for planned_bin in plan.bins:
        bin_image          = Image.new('RGBA', planned_bin.actual_size, (0, 0, 0, 0))
        bin_alphamap_image = None

        for sprite in planned_bin.sprites:
            sprite.infer_rotation()

            paste_coord = sprite.texture_region.offset
            bin_image.paste(sprite.image, paste_coord)

            if sprite.alphamap_image is not None:
                if bin_alphamap_image is None:
                    bin_alphamap_image = Image.new('L', planned_bin.actual_size, 255)
                bin_alphamap_image.paste(sprite.alphamap_image, paste_coord)

            sprite.texture_region /= planned_bin.actual_size
            write_sprite_def(temp_dst / f'{sprite.name}.spr', sprite.dump_spritedef_dict())
            sprite.unload()

        texture_id       = f'atlas_{planned_bin.texture_id}'
        dstfile          = temp_dst / f'{texture_id}.png'
        dstfile_alphamap = temp_dst / f'{texture_id}.alphamap.png'
        dstfile_meta     = temp_dst / f'{texture_id}.tex'

        write_texture_def(
            dstfile_meta,
            texture          = texture_id,
            texture_fmt      = tex_format,
            global_overrides = texture_global_overrides,
            local_overrides  = texture_local_overrides,
            alphamap_tex_fmt = 'png' if bin_alphamap_image is not None else None,
        )

        def process(bin_image=bin_image, dstfile=dstfile, tex_format=tex_format):
            bin_image.save(dstfile)
            bin_image.close()

            if dstfile.suffix[1:].lower() != tex_format:
                new_dstfile = dstfile.with_suffix(f'.{tex_format}')
                if tex_format == 'webp':
                    subprocess.check_call([
                        'cwebp', '-progress', '-preset', 'icon',
                        '-z', '9', '-lossless', '-q', '100', '-m', '6',
                        str(dstfile), '-o', str(new_dstfile),
                    ])
                else:
                    raise TaiseiError(f'Unhandled conversion {dstfile.suffix[1:]} -> {tex_format}')
                dstfile.unlink()

        futures.append(executor.submit(process))

        if bin_alphamap_image is not None:
            def process_alphamap(img=bin_alphamap_image, path=dstfile_alphamap):
                img.save(path)
                img.close()
            futures.append(executor.submit(process_alphamap))

    wait_for_futures(futures)

    pattern = re.compile(
        rf'^atlas_{re.escape(plan.atlasname)}_\d+(?:\.alphamap)?'
        rf'\.({"|".join(texture_formats + ["tex"])})$'
    )
    for path in dst.iterdir():
        if pattern.match(path.name):
            path.unlink()

    targets = list(temp_dst.glob('**/*'))
    for d in (p.relative_to(temp_dst) for p in targets if p.is_dir()):
        (dst / d).mkdir(parents=True, exist_ok=True)
    for f in (p.relative_to(temp_dst) for p in targets if not p.is_dir()):
        shutil.copyfile(str(temp_dst / f), str(dst / f))


# ---------------------------------------------------------------------------
# TSAtlas mode: pack into a .tsatlas file
#
# File layout (little-endian, packed, no padding):
#     uint64  magic               (TSATLAS_MAGIC)
#     uint16  version             (TSATLAS_VERSION)
#     uint16  width               (bin pixel width)
#     uint16  height              (bin pixel height)
#     uint16  num_sprites
#     uint32  flags               (reserved, must be 0)
#     Sprite  sprites[num_sprites]
#     uint8   data[]              (concatenated raw pixel data, same order as sprites[])
#
# Each Sprite entry:
#     uint8   name_len
#     char    name[name_len]      (UTF-8, not null-terminated)
#     uint32  flags               (see TSAtlasSpriteFlag)
#     uint16  offset_x            (pixels)
#     uint16  offset_y            (pixels)
#     uint16  width               (pixels)
#     uint16  height              (pixels)
#     uint16  tex_offset_x        (denormalized texels)
#     uint16  tex_offset_y        (denormalized texels)
#     uint16  tex_width           (denormalized texels)
#     uint16  tex_height          (denormalized texels)
#     float   virtual_width
#     float   virtual_height
#     float   pad_top
#     float   pad_bottom
#     float   pad_left
#     float   pad_right
#
# Pixel data is stored as 8-bit premultiplied RGBA, unless FILTER_SUBTRACT_GREEN
# flag is set; see TSAtlasSpriteFlag for details.
# ---------------------------------------------------------------------------

TSATLAS_MAGIC = 0xa7baacaab2b6baad
TSATLAS_VERSION = 1

class TSAtlasSpriteFlag(enum.IntFlag):
    # Pixel data is stored as (A, G, R', B'), where:
    #   A  = Alpha
    #   G  = Green
    #   R' = (Red  - Green) % 255
    #   B' = (Blue - Green) % 255
    # This filter is designed to improve zstd compression ratios.
    FILTER_SUBTRACT_GREEN  = (1 << 0)

    # Sprite is rotated 90 degrees counter-clockwise.
    # The pixel data is stored in the rotated form.
    # Virtual size and paddings are NOT rotated.
    ROTATED                = (1 << 1)

TSATLAS_SPRITE_FLAG_FILTER_SUBTRACT_GREEN  = (1 << 0)
TSATLAS_SPRITE_FLAG_ROTATED                = (1 << 1)


def _pack_sprite_entry(sprite):
    assert sprite.image.mode == 'RGBA'

    name_bytes = sprite.name.encode('utf-8')
    assert len(name_bytes) <= 255, f"Sprite name too long: '{sprite.name}'"

    tr = sprite.texture_region
    ox, oy = tr.offset
    pad = sprite.padding
    vw, vh = sprite.virtual_size

    flags = 0

    if sprite.filtered:
        flags |= TSAtlasSpriteFlag.FILTER_SUBTRACT_GREEN

    if sprite.rotated:
        flags |= TSAtlasSpriteFlag.ROTATED

    return (
        struct.pack('<B', len(name_bytes)) + name_bytes +
        struct.pack('<I8H6f',
            flags,
            ox, oy, *sprite.image.size,
            ox, oy, tr.width, tr.height,
            vw, vh,
            pad.top, pad.bottom, pad.left, pad.right,
        )
    )


def execute_work_plan_tsatlas(plan, args):
    dst = args.dest_dir
    sort_fn = TSATLAS_SORT_STRATEGIES[args.tsatlas_sort][0]
    dst.mkdir(parents=True, exist_ok=True)
    MiB = 1024 * 1024

    for planned_bin in plan.bins:
        w, h    = planned_bin.actual_size
        sprites = planned_bin.sprites

        assert len(sprites) <= 0xFFFF, \
            f"Too many sprites in bin '{planned_bin.texture_id}' ({len(sprites)} > 65535)"

        if sort_fn is not None:
            sort_fn(sprites)

        header  = struct.pack('<QHHHHI', TSATLAS_MAGIC, TSATLAS_VERSION, w, h, len(sprites), 0)
        outpath = dst / f'{planned_bin.texture_id}.tsatlas.zst'

        with outpath.open('wb') as f_backing, \
             zstd.ZstdFile(f_backing, 'wb', options={
                 zstd.CompressionParameter.compression_level: args.tsatlas_zstd_level,
                 zstd.CompressionParameter.enable_long_distance_matching: True,
             }) as f:

            size = f.write(header)
            for sprite in sprites:
                assert sprite._premultiplied
                assert sprite.image.mode == 'RGBA'
                sprite.infer_rotation()
                size += f.write(_pack_sprite_entry(sprite))

            n = len(sprites)
            for i, sprite in enumerate(sprites):
                print(f'[{i+1:4d} / {n:4d}]  {sprite.name}', end='', flush=True)

                size += f.write(sprite.image.tobytes())
                sprite.unload()

                comp = f_backing.tell()
                pad  = max(0, 46 - len(sprite.name)) * ' '
                print(f'{pad}{100 * comp / size:5.02f}%  {comp / MiB:.2f}MiB')

                write_sprite_def(dst / f'{sprite.name}.spr', {'atlas': planned_bin.texture_id})

        print(f'Written {outpath}')
        print(f'{comp / MiB:.2f}MiB / {size / MiB:.2f}MiB, ratio: {100 * comp / size:.02f}%')


# ---------------------------------------------------------------------------
# gen_atlas entry point
# ---------------------------------------------------------------------------

def _noop():
    pass


def _force_spawn_workers(ex):
    # Spawn all workers on the main thread before any threads exist,
    # to avoid a deadlock with fork-based multiprocessing.
    for _ in as_completed(ex.submit(_noop) for _ in range(ex._max_workers)):
        pass


def gen_atlas(args):
    config_dir    = args.config_dir
    src           = args.source_dir
    dst           = args.dest_dir
    binsize       = align_size((args.width, args.height))
    max_binsize   = align_size((args.max_width, args.max_height))

    texture_local_overrides  = keyval.parse(src / 'atlas.tex', missing_ok=True)
    texture_global_overrides = keyval.parse(config_dir / 'atlas.tex', missing_ok=True)

    def get_border(sprite):
        return max(args.border, int(sprite.config.get('border', args.border)))

    rects   = {}
    sprites = {}
    sprite_futures     = []
    preproc_futures    = []

    with ExitStack() as stack:
        if gil_enabled():
            fork_ex   = stack.enter_context(ProcessPoolExecutor())
            _force_spawn_workers(fork_ex)
            thread_ex = stack.enter_context(ThreadPoolExecutor())
        else:
            thread_ex = stack.enter_context(ThreadPoolExecutor())
            fork_ex   = thread_ex

        for path in sprite_sources_iter(src):
            name        = path.relative_to(src).with_suffix('').as_posix()
            config_path = config_dir / atlas.get_sprite_config_file_name(name)
            sprite_futures.append(thread_ex.submit(atlas.Sprite.load, name, path, config_path))

        for future in as_completed(sprite_futures):
            sprite = future.result()
            sprites[sprite.name] = sprite
            b = get_border(sprite)
            rects[sprite.name] = (
                sprite.image.size[0] + b * 2,
                sprite.image.size[1] + b * 2,
                sprite.name,
            )
            if args.format == TSATLAS_FORMAT:
                preproc_futures.append(thread_ex.submit(sprite.preprocess_for_tsatlas))

        pack_result = pack_rects_brute_force(
            executor   = fork_ex,
            atlasname  = args.name,
            rects      = tuple(rects.values()),
            settings   = PackSettings(
                bin_size = binsize,
                bin_max_size = max_binsize,
                sweep_samples = args.samples,
                single_bin = args.single
            )
        )
        wait_for_futures(preproc_futures)

        if not pack_result.success:
            missing = len(rects) - pack_result.num_images_packed
            raise TaiseiError(
                f'{missing} sprite{"s were" if missing > 1 else " was"} not packed '
                f'(bin size too small?)'
            )

        plan = build_work_plan(
            pack_result = pack_result,
            sprites     = sprites,
            atlasname   = args.name,
            crop        = args.crop,
            border      = args.border,
        )

        for i, b in enumerate(plan.bins):
            w, h = b.actual_size
            print(f'[{args.name}] Bin #{i}: {w} x {h} == {w * h}')

        if args.format == TSATLAS_FORMAT:
            execute_work_plan_tsatlas(plan, args)
        else:
            with TemporaryDirectory(prefix=f'taisei-atlas-{args.name}') as tmp:
                execute_work_plan_image_files(
                    executor                 = thread_ex,
                    plan                     = plan,
                    dst                      = dst,
                    temp_dst                 = Path(tmp),
                    tex_format               = args.format,
                    texture_global_overrides = texture_global_overrides,
                    texture_local_overrides  = texture_local_overrides,
                )


def upgrade_configs(overrides_dir, src_dir, config_dir):
    for path in sprite_sources_iter(src_dir):
        name         = path.relative_to(src_dir).with_suffix('').as_posix()
        config_fname = atlas.get_sprite_config_file_name(name)
        config_path  = config_dir / config_fname

        if config_path.is_file():
            continue

        sprite = atlas.Sprite.load(name, path, overrides_dir / config_fname, force_autotrim=False)
        sprite.upgrade_config()

        if sprite.config:
            config_path.parent.mkdir(parents=True, exist_ok=True)
            update_text_file(config_path, keyval.dump(sprite.config))
            print(f'{sprite.name}:  {sprite.config!r}')

        sprite.unload()


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main(args):
    parser = argparse.ArgumentParser(
        description='Generate texture atlases and sprite definitions',
        prog=args[0],
    )

    parser.add_argument('config_dir', type=Path,
        help='Directory containing per-sprite configuration files')
    parser.add_argument('source_dir', type=Path,
        help='Directory containing input textures (searched recursively)')
    parser.add_argument('dest_dir', type=Path,
        help='Directory to dump the results into')

    parser.add_argument('--name',   '-n', type=str, default=None,
        help='Atlas identifier (default: inferred from source directory name)')

    parser.add_argument('--width',  '-W', type=int, default=2048, metavar='W',
        help='Base atlas bin width (default: 2048)')
    parser.add_argument('--height', '-H', type=int, default=2048, metavar='H',
        help='Base atlas bin height (default: 2048)')
    parser.add_argument('--max-width', type=int, default=8192, metavar='W',
        help='Maximum atlas bin width (default: 8192)')
    parser.add_argument('--max-height', type=int, default=8192, metavar='H',
        help='Maximum atlas bin height (default: 8192)')
    parser.add_argument('--samples', type=int, default=0, metavar='NUM',
        help='How many intermediate sizes to try; increases quality but very slow (default: 0, base size only)')

    parser.add_argument('--border', '-b', type=int, default=1, metavar='WIDTH',
        help='Protective border width around each sprite in pixels (default: 1)')

    parser.add_argument('--crop',    '-c', dest='crop', action='store_true',  default=True,
        help='Remove unused space from bins (default)')
    parser.add_argument('--no-crop', '-C', dest='crop', action='store_false',
        help='Do not remove unused space from bins')

    parser.add_argument('--single',   '-s', dest='single', action='store_true',  default=True,
        help='Pack everything into a single texture, growing as needed (default)')
    parser.add_argument('--multiple', '-m', dest='single', action='store_false',
        help='Split across multiple textures if needed')

    parser.add_argument('--format', '-f',
        choices=texture_formats + [TSATLAS_FORMAT],
        default=texture_formats[0],
        help=f'Output format (default: {texture_formats[0]}; '
             f'{TSATLAS_FORMAT!r} produces zstd-compressed binary sprite sheets)')

    parser.add_argument('--tsatlas-sort',
        choices=list(TSATLAS_SORT_STRATEGIES.keys()),
        default=TSATLAS_SORT_DEFAULT,
        metavar='STRATEGY',
        help=f'Sprite sort order for {TSATLAS_FORMAT} output (default: {TSATLAS_SORT_DEFAULT}; '
             f'choices: {", ".join(TSATLAS_SORT_STRATEGIES)})')
    parser.add_argument('--tsatlas-zstd-level', type=int, default=3, metavar='LEVEL',
        help='zstd compression level for tsatlas output (default: 3)')

    parser.add_argument('--upgrade-configs', action='store_true', default=False,
        help='Migrate old-style override configs to new format. '
             'config_dir is treated as the overrides root; results go into dest_dir. '
             'Existing configs are not overwritten.')

    args = parser.parse_args()

    if args.name is None:
        args.name = args.source_dir.name

    args.config_dir = args.config_dir.resolve()
    args.source_dir = args.source_dir.resolve()
    args.dest_dir   = args.dest_dir.resolve()

    if args.upgrade_configs:
        upgrade_configs(args.config_dir, args.source_dir, args.dest_dir)
    else:
        gen_atlas(args)


if __name__ == '__main__':
    run_main(main)
