# Taisei

## Introduction

Taisei is an open clone of the Tōhō Project series. Tōhō is a one-man project of
shoot-em-up games set in an isolated world full of Japanese folklore.

## Installation

Dependencies:
* SDL2 >= 2.0.5, SDL2\_ttf, SDL2\_mixer
* zlib
* libzip >= 1.0
* libpng >= 1.5.0
* OpenGL >= 2.1
* CMake >= 3.3 (build system)
* pkg-config (build dependency)

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
```

`RELATIVE` is always set when building for Windows or macOS.

## Where are my replays, screenshots and settings?

Taisei stores all data in a platform-specific directory:

* On **Windows**, this will probably be `%APPDATA%\taisei`
* On **macOS**, it's `$HOME/Library/Application Support/taisei`
* On **Linux**, **\*BSD**, and most other **Unix**-like systems, it's `$XDG_DATA_HOME/taisei` or `$HOME/.local/share/taisei`

You can set the environment variable `TAISEI_STORAGE_PATH` to override this behaviour.

## Game controller support

Taisei uses SDL2's unified GameController API. This allows us to correctly support any device that SDL recognizes by default, while treating all of them the same way. This also means that if your device is not supported by SDL, you will not be able to use it unless you provide a custom mapping. If your controller is listed in the settings menu, then you're fine. If not, read on.

An example mapping string looks like this:
```
03000000ba2200002010000001010000,Jess Technology USB Game Controller,a:b2,b:b1,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:,leftshoulder:b4,lefttrigger:b6,leftx:a0,lefty:a1,rightshoulder:b5,righttrigger:b7,rightx:a3,righty:a2,start:b9,x:b3,y:b0,
```

There are a few ways to generate a custom mapping:
* You can use the [`controllermap`](https://aur.archlinux.org/packages/controllermap) utility, which [comes with SDL source code](https://hg.libsdl.org/SDL/file/68a767ae3a88/test/controllermap.c).
* If you use Steam, you can configure your controller there. Then you can add Taisei as a non-Steam game; run it from Steam and everything should *just work™*. In case you don't want to do that, find `config/config.vdf` in your Steam installation directory, and look for the `SDL_GamepadBind` variable. It contains a list of SDL mappings separated by line breaks.
* You can also try the [SDL2 Gamepad Tool by General Arcade](http://www.generalarcade.com/gamepadtool/). This program is free to use, but not open source.
* Finally, you can try to write a mapping by hand. You will probably have to refer to the SDL documentation. See [misc/gamecontrollerdb/gamecontrollerdb.txt](gamecontrollerdb.txt) for some more examples.

Once you have your mapping, there are two ways to make Taisei use it:
* Create a file named `gamecontrollerdb.txt` where your config, replays and screenshots are, and put each mapping on a new line.
* Put your mappings in the environment variable `SDL_GAMECONTROLLERCONFIG`, also separated by line breaks. Other games that use the GameController API will also pick them up.

When you're done, please consider contributing your mappings to [SDL](https://libsdl.org/), [SDL_GameControllerDB](https://github.com/gabomdq/SDL_GameControllerDB), and [us](https://github.com/laochailan/taisei/issues), so that other people can benefit from your work.

Also note that we currently only handle input from analog axes and digital buttons. Hats, analog buttons, and anything more exotic will not work, unless remapped.

## Background music

Currently Taisei does not include any background music. To use this feature,
you should have the required audio files in the `bgm/` subdirectory of global
resource directory (for example, `$prefix/share/taisei/bgm/`), or better,
in user-specific resource directory (`$HOME/.local/share/taisei/resources/bgm/`).
BGM (as well as SFX) may be in `.wav`, `.flac`, or `.ogg` format; additionally
you may try other formats such as `.mp3`, `.aiff`, `.mod`, `.xm`, etc. if your
build of SDL2_mixer supports these formats.

The complete music pack consists of 16 tracks:
```
    credits     BGM for credits screen
    ending      BGM for ending
    gameover    BGM for game over screen
    menu        BGM for menus (excluding in-game menu which pauses BGM)
    stageN      N=1..6, standard stage theme
    stageNboss  N=1..6, boss theme for a stage
```

If you want to specify stage/boss theme names to be shown during gameplay, you
may do it by editing the [`resources/bgm/bgm.conf`](bgm/bgm.conf) file. It can
be done either before building Taisei (then modified file will be packed along
with other game resources if `PACKAGE_DATA` is on), or after installation (then
modified `bgm.conf` will override packed one; in that case it should be placed
in any of the `bgm/` directories mentioned above).

## Sound problems (Linux)

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
