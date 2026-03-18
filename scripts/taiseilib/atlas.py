
import collections.abc
import dataclasses
import numpy as np
import operator
import re
import typing

from typing import Optional
from PIL import Image

from . import keyval


SPRITE_SUFFIXES = ('.png', '.webp')
DEFAULT_SPRITE_SCALE = 0.5


@dataclasses.dataclass(slots=True, frozen=True)
class Geometry:
    width: float
    height: float
    offset_x: float = 0.0
    offset_y: float = 0.0

    regex: typing.ClassVar = re.compile(r'(\d+)x(\d+)([+-]\d+)([+-]\d+)')

    @classmethod
    def from_string(cls, string):
        return cls(*(int(s) for s in cls.regex.findall(string)[0]))

    @classmethod
    def from_bbox(cls, bbox):
        left, upper, right, lower = bbox
        return cls(right - left, lower - upper, left, upper)

    @property
    def offset(self):
        return self.offset_x, self.offset_y

    @property
    def extent(self):
        return self.width, self.height

    def _apply_op(self, other, op):
        if isinstance(other, collections.abc.Sequence) and len(other) == 2:
            mx = other[0]
            my = other[1]
            return Geometry(
                width=op(self.width, mx),
                height=op(self.height, my),
                offset_x=op(self.offset_x, mx),
                offset_y=op(self.offset_y, my),
            )

        return Geometry(
            width=op(self.width, other),
            height=op(self.height, other),
            offset_x=op(self.offset_x, other),
            offset_y=op(self.offset_y, other),
        )

    def __mul__(self, other): return self._apply_op(other, operator.mul)
    def __truediv__(self, other): return self._apply_op(other, operator.truediv)

    def __str__(self):
        return '{}x{}{:+}{:+}'.format(self.width, self.height, self.offset_x, self.offset_y)


@dataclasses.dataclass(slots=True, frozen=True)
class Padding:
    top: float = 0.0
    bottom: float = 0.0
    left: float = 0.0
    right: float = 0.0

    @classmethod
    def from_offset(cls, x=0, y=0):
        return cls(top=y, bottom=-y, left=x, right=-x)

    @classmethod
    def relative(cls, g_base, g_object):
        top = g_object.offset_y - g_base.offset_y
        bottom = (g_base.offset_y + g_base.height) - (g_object.offset_y + g_object.height)
        left = g_object.offset_x - g_base.offset_x
        right = (g_base.offset_x + g_base.width) - (g_object.offset_x + g_object.width)
        return cls(top, bottom, left, right)

    @classmethod
    def from_config(cls, conf):
        return cls(
            top=float(conf.get('padding_top', 0.0)),
            bottom=float(conf.get('padding_bottom', 0.0)),
            left=float(conf.get('padding_left', 0.0)),
            right=float(conf.get('padding_right', 0.0))
        )

    def _apply_op(self, other, op):
        if not isinstance(other, Padding):
            other = Padding(other, other, other, other)

        return Padding(
            top=op(self.top, other.top),
            bottom=op(self.bottom, other.bottom),
            left=op(self.left, other.left),
            right=op(self.right, other.right)
        )

    def __add__(self, other): return self._apply_op(other, operator.add)
    def __sub__(self, other): return self._apply_op(other, operator.sub)
    def __mul__(self, other): return self._apply_op(other, operator.mul)
    def __truediv__(self, other): return self._apply_op(other, operator.truediv)

    def export(self, conf):
        for side in ('top', 'bottom', 'left', 'right'):
            key = 'padding_' + side
            val = getattr(self, side)

            if val:
                conf[key] = '{:.17g}'.format(val)
            elif key in conf:
                del conf[key]


def find_alphamap(basepath):
    for fmt in SPRITE_SUFFIXES:
        mpath = basepath.with_suffix(f'.alphamap{fmt}')

        if mpath.is_file():
            return mpath


def get_sprite_config_file_name(basename):
    if re.match(r'.*\.frame\d{4}$', basename):
        gname, _ = basename.rsplit('.', 1)
        gname += '.framegroup'
        return f'{gname}.spr'

    return f'{basename}.spr'


@dataclasses.dataclass(eq=False)
class Sprite:
    name: str
    image: Optional[Image.Image]
    alphamap_image: Optional[Image.Image] = None
    padding: Padding = dataclasses.field(default_factory=Padding)
    config: dict[str, str] = dataclasses.field(default_factory=dict)
    texture_id: str = ''
    texture_region: Geometry = Geometry(0, 0)
    rotated: bool = False

    def __post_init__(self):
        self._trimmed = False
        self._premultiplied = False
        self.filtered = False
        self._check_alphamap()

    def _check_alphamap(self):
        if self.alphamap_image is not None:
            assert self.image is not None
            assert self.image.size == self.alphamap_image.size

    @classmethod
    def load(cls, name, image_path, config_path, force_autotrim=None, default_autotrim=True):
        config = keyval.parse(config_path, missing_ok=True)
        image = Image.open(image_path)
        alphamap_path = find_alphamap(image_path)

        if alphamap_path is not None:
            tmp = Image.open(alphamap_path)
            alphamap_image, *_ = tmp.split()  # Extract red channel
            tmp.close()
        else:
            alphamap_image = None

        sprite = cls(
            name=name,
            image=image,
            alphamap_image=alphamap_image,
            padding=Padding.from_config(config),
            config=config,
        )

        if force_autotrim is not None:
            autotrim = force_autotrim
        else:
            autotrim = keyval.strbool(config.get('autotrim', default_autotrim))

        if autotrim:
            sprite.trim()

        return sprite

    def upgrade_config(self):
        image_w = self.image.size[0]
        virtual_w = float(self.config.get('w', image_w))
        scale_factor = virtual_w / image_w

        if scale_factor != DEFAULT_SPRITE_SCALE:
            self.config['scale'] = '{:.17g}'.format(scale_factor)

        def trydel(k):
            try:
                del self.config[k]
            except KeyError:
                pass

        for key in ('w', 'h'):
            trydel(key)

        self.padding.export(self.config)

    @property
    def scale(self):
        return float(self.config.get('scale', DEFAULT_SPRITE_SCALE))

    @property
    def virtual_size(self):
        assert self.image is not None
        w, h = self.image.size
        if self.rotated:
            w, h = h, w
        scale = self.scale
        w *= scale
        h *= scale
        return w, h

    def trim(self):
        if self._trimmed:
            return

        assert self.image is not None

        bbox = self.image.getbbox()
        trimmed = self.image.crop(bbox)

        g_base = Geometry(*self.image.size)
        g_trimmed = Geometry.from_bbox(bbox)
        self.padding = self.padding + Padding.relative(g_base, g_trimmed) * self.scale

        self.image.close()
        self.image = trimmed

        if self.alphamap_image is not None:
            alphamap_trimmed = self.alphamap_image.crop(bbox)
            self.alphamap_image.close()
            self.alphamap_image = alphamap_trimmed
            self._check_alphamap()

        self._trimmed = True

    def premultiply(self):
        if self._premultiplied:
            return

        assert self.image is not None

        if self.image.mode != 'RGBA':
            self.image = self.image.convert('RGBA')

        img_array = np.asarray(self.image)
        self.image.close()
        self.image = None

        dtype = img_array.dtype
        max_val = np.float32(np.iinfo(dtype).max if np.issubdtype(dtype, np.integer) else 1.0)
        assert dtype == np.uint8

        # Normalise to [0, 1] in float32 - sufficient for 8-bit and 16-bit sources
        color = img_array[..., :-1].astype(np.float32) / max_val
        alpha = img_array[..., -1:].astype(np.float32) / max_val
        del img_array

        color *= alpha
        np.clip(color, 0.0, 1.0, out=color)

        if self.alphamap_image is not None:
            if self.alphamap_image.mode != 'L':
                self.alphamap_image = self.alphamap_image.convert('L')

            alphamap_array = np.asarray(self.alphamap_image)
            self.alphamap_image.close()
            self.alphamap_image = None

            assert alphamap_array.dtype == dtype
            alpha[..., 0] *= alphamap_array.astype(np.float32) / max_val

        color *= max_val
        np.clip(color, 0.0, max_val, out=color)
        alpha *= max_val
        np.clip(alpha, 0.0, max_val, out=alpha)

        result = np.concatenate([color.astype(dtype), alpha.astype(dtype)], axis=-1)
        del color, alpha

        self.image = Image.fromarray(result, mode='RGBA')
        self._premultiplied = True

    def filter_color(self):
        assert not self.filtered
        assert self.image is not None
        data = np.asarray(self.image)
        self.image.close()
        shape = data.shape
        px  = data.reshape(-1, 4)
        r, g, b, a = px[:,0], px[:,1], px[:,2], px[:,3]
        out = np.stack([a, g, r - g, b - g], axis=1)
        out = out.reshape(*shape)
        assert out.shape == data.shape
        self.image = Image.fromarray(out, mode='RGBA')
        self.filtered = True

    def preprocess_for_tsatlas(self):
        self.premultiply()
        self.filter_color()

    def dump_spritedef_dict(self):
        d = {}

        if self.texture_id is not None:
            d['texture'] = 'atlas_' + str(self.texture_id)

        if self.texture_region is not None:
            d['region_x'] = '{:.17f}'.format(self.texture_region.offset_x)
            d['region_y'] = '{:.17f}'.format(self.texture_region.offset_y)
            d['region_w'] = '{:.17f}'.format(self.texture_region.width)
            d['region_h'] = '{:.17f}'.format(self.texture_region.height)

        w, h = self.virtual_size
        d['w'] = '{:g}'.format(w)
        d['h'] = '{:g}'.format(h)

        if self.rotated:
            d['rotated'] = 'yes'

        self.padding.export(d)

        return d

    def rotate(self):
        assert not self.rotated
        assert self.image is not None

        rotated_img = self.image.transpose(Image.Transpose.ROTATE_90)
        self.image.close()
        self.image = rotated_img

        if self.alphamap_image is not None:
            rotated_img = self.alphamap_image.transpose(Image.Transpose.ROTATE_90)
            self.alphamap_image.close()
            self.alphamap_image = rotated_img

        self.rotated = True

    def infer_rotation(self):
        assert self.image is not None
        if self.image.size != self.texture_region.extent:
            rotated_extent = self.image.size[1], self.image.size[0]
            assert self.texture_region.extent == rotated_extent
            self.rotate()

    def unload(self):
        if self.image is not None:
            self.image.close()
            self.image = None

        if self.alphamap_image is not None:
            self.alphamap_image.close()
            self.alphamap_image = None

    def __del__(self):
        self.unload()
