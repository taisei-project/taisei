 Taisei

## Introduction

Taisei is an open clone of the Tōhō Project series. Tōhō is a one-man project of
shoot-em-up games set in an isolated world full of Japanese folklore.

## Installation

Dependencies:
* SDL2, SDL2\_ttf, SDL2\_mixer
* libpng, ZLIB
* OpenGL
* CMake (build system)

To build and install Taisei just follow these steps.

```
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=$yourprefix ..
make
make install
```

This will install game data to `$prefix/share/taisei/` and build this path
_statically_ into the executable. This might be a package maintainer’s choice.
Alternatively you may want to add `-DRELATIVE=TRUE` to get a relative structure
like

```
$prefix/taisei
$prefix/data/
($prefix/config)
```

`RELATIVE` is always set when building for Windows.

## NOTE ABOUT REPLAYS

As of version 1.0 replays are not guaranteed to work between different
operating systems or architectures (or compiler versions). Screen capture
your replays if you really want to publish them.

## NOTE ABOUT BACKGROUND MUSIC

Currently Taisei does not include any background music. To use this feature,
you should have required audio files in `bgm/` subdirectory.
BGM (as well as SFX) may be in `.wav`, `.flac`, or `.ogg` format; additionally
you may try another formats such as `.mp3`, `.aiff`, `.mod`, `.xm`, etc. if
your build of SDL2_mixer supports these formats.

Complete music pack consists of 16 bgm\_\*.(ogg/wav/flac) files, where ‘\*’ mean:
```
	credits		BGM for credits screen
	ending		BGM for ending
	gameover	BGM for game over screen
	menu		BGM for menus (excluding in-game menu which pauses BGM)
	stageN		N=1..6, standard stage theme
	stageNboss	N=1..6, boss theme for a stage
```

If you want to specify stage/boss theme names to be shown during gameplay, you
may do it in `bgm/bgm.conf` file. This file contains some lines, each of which
consists of bgm filename (without extension), space of tab, and theme name.
No space/tab allowed either in beginning of line or BGM filenames listed in
this file; theme names may contain them.

## Sound problems

If your sound becomes glitchy, and you encounter lot of console messages like:
`ALSA lib pcm.c:7234:(snd_pcm_recover) underrun occurred`,
it seems like you possibly have broken ALSA configuration.
This may be fixed by playing with parameter values of `pcm.dmixer.slave` option
group in `/etc/asound.conf` or wherever you have your ALSA configuration.
Commenting `period_time`, `period_size`, `buffer_size`, `rate` may give you
the first approach to what to do.

## Contact

http://taisei-project.org/

\#taisei-project on Freenode
