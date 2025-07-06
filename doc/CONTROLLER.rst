Taisei Project - Controller Settings
====================================

How to use controllers, such as gamepads, with Taisei.

.. contents::

Controller Support
------------------

Supported Controllers
"""""""""""""""""""""

Taisei uses SDL2’s unified `GameController API <https://wiki.libsdl.org/CategoryGameController>`__. This allows us to
correctly support any device that SDL recognizes by default, while treating all of them the same way. The vast majority
of USB Human Interface Devices should be supported by this method, especially ones made in the last 15 years or so, once
the overall USB ecosystem began to stabilize.

Unsupported Controllers
"""""""""""""""""""""""

If, for some reason, your device isn’t supported by SDL2’s GameController API, such as a particularly old or perhaps
custom-made game controller using a unique interface, you’ll need to provide it a custom controller mapping.

There are a few ways to generate a custom mapping:

- You can use the `controllermap <https://aur.archlinux.org/packages/controllermap>`__ utility, which `comes with SDL
  source code <https://hg.libsdl.org/SDL/file/68a767ae3a88/test/controllermap.c>`__.
- If you use Steam, you can configure your controller there. Then you can add Taisei as a non-Steam game; run it from
  Steam and everything should *just work™*. In case you don’t want to do that, find ``config/config.vdf`` in your Steam
  installation directory, and look for the ``SDL_GamepadBind`` variable. It contains a list of SDL mappings separated by
  line breaks.
- You can also try the `SDL2 Gamepad Tool by General Arcade <http://www.generalarcade.com/gamepadtool/>`__. This program
  is free to use, but not open source.
- Finally, you can try to write a mapping by hand. You will probably have to refer to the SDL documentation. See
  `gamecontrollerdb.txt <https://github.com/taisei-project/SDL_GameControllerDB/blob/master/gamecontrollerdb.txt>`__ for
  some more examples.

Whether you use one of the above tools, or hand-roll one yourself, the mapping should look something like this:

::

   03000000ba2200002010000001010000,Jess Technology USB Game Controller,a:b2,b:b1,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:,leftshoulder:b4,lefttrigger:b6,leftx:a0,lefty:a1,rightshoulder:b5,righttrigger:b7,rightx:a3,righty:a2,start:b9,x:b3,y:b0,

Once you have your mapping, there are two ways to make Taisei use it:

- Create a file named ``gamecontrollerdb.txt`` where your config, replays and screenshots are, and put each mapping on a
  new line.
- Put your mappings in the environment variable ``SDL_GAMECONTROLLERCONFIG``, also separated by line breaks. Other games
  that use the GameController API will also pick them up.

When you’re done, please consider contributing your mappings to `SDL <https://libsdl.org/>`__, `SDL_GameControllerDB
<https://github.com/gabomdq/SDL_GameControllerDB>`__, and `us
<https://github.com/taisei-project/SDL_GameControllerDB>`__, so that other people can benefit from your work.

Also note that we currently only handle input from analog axes and analog/digital buttons. Anything more exotic, such as
hats, probably won’t work out of the box.
