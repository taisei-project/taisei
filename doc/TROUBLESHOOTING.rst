Troubleshooting
---------------

Game controller support
-----------------------

Taisei uses SDL2's unified GameController API. This allows us to correctly
support any device that SDL recognizes by default, while treating all of them
the same way. This also means that if your device is not supported by SDL, you
will not be able to use it unless you provide a custom mapping. If your
controller is listed in the settings menu, then you're fine. If not, read on.

An example mapping string looks like this:

::

    03000000ba2200002010000001010000,Jess Technology USB Game Controller,a:b2,b:b1,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:,leftshoulder:b4,lefttrigger:b6,leftx:a0,lefty:a1,rightshoulder:b5,righttrigger:b7,rightx:a3,righty:a2,start:b9,x:b3,y:b0,

There are a few ways to generate a custom mapping:

-  You can use the
   `controllermap <https://aur.archlinux.org/packages/controllermap>`__ utility,
   which `comes with SDL source code
   <https://hg.libsdl.org/SDL/file/68a767ae3a88/test/controllermap.c>`__.
-  If you use Steam, you can configure your controller there. Then you can add
   Taisei as a non-Steam game; run it from Steam and everything should *just
   work™*. In case you don't want to do that, find ``config/config.vdf`` in your
   Steam installation directory, and look for the ``SDL_GamepadBind`` variable.
   It contains a list of SDL mappings separated by line breaks.
-  You can also try the `SDL2 Gamepad Tool by General Arcade
   <http://www.generalarcade.com/gamepadtool/>`__. This program is free to use,
   but not open source.
-  Finally, you can try to write a mapping by hand. You will probably have to
   refer to the SDL documentation. See `gamecontrollerdb.txt
   <misc/gamecontrollerdb/gamecontrollerdb.txt>`__ for some more examples.

Once you have your mapping, there are two ways to make Taisei use it:

-  Create a file named ``gamecontrollerdb.txt`` where your config, replays and
   screenshots are, and put each mapping on a new line.
-  Put your mappings in the environment variable ``SDL_GAMECONTROLLERCONFIG``,
   also separated by line breaks. Other games that use the GameController API
   will also pick them up.

When you're done, please consider contributing your mappings to
`SDL <https://libsdl.org/>`__,
`SDL_GameControllerDB <https://github.com/gabomdq/SDL_GameControllerDB>`__,
and `us <https://github.com/taisei-project/SDL_GameControllerDB>`__, so
that other people can benefit from your work.

Also note that we currently only handle input from analog axes and digital
buttons. Hats, analog buttons, and anything more exotic will not work, unless
remapped.

Sound problems (Linux)
^^^^^^^^^^^^^^^^^^^^^^

If your sound becomes glitchy, and you encounter lot of console messages like:

::

    ALSA lib pcm.c:7234:(snd_pcm_recover) underrun occurred

it seems like you possibly have broken ALSA configuration. This may be fixed by
playing with parameter values of ``pcm.dmixer.slave`` option group in
``/etc/asound.conf`` or wherever you have your ALSA configuration.
Commenting ``period_time``, ``period_size``, ``buffer_size``, ``rate`` may give
you the first approach to what to do.


Framerate Issues on macOS
^^^^^^^^^^^^^^^^^^^^^^^^^

If you are experiencing framerate issues on macOS with AMD graphics cards,
it's likely due to a
`known issue <https://github.com/taisei-project/taisei/issues/182>`__. However,
it should be fixed at this point, but if you still experience it, please
`open a new issue <https://github.com/taisei-project/taisei/issues/>`__ and tell
us what make and model of Mac you're experiencing the issue with, as well as any
logs that might help us.
