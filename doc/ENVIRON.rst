Taisei Project - Environment Variables
======================================

.. contents::

Introduction
------------

Some of Taisei’s more advanced configuration options are delegated to `environment variables
<https://en.wikipedia.org/wiki/Environment_variable>`__. This document attempts to describe them all.

Generally you don’t need to set any of them. They are intended for developers and advanced users only. Therefore,
familiarity with the concept is assumed.

In addition to the variables listed here, those processed by our runtime dependencies (such as SDL) also take effect,
unless stated otherwise.

Variables
---------

Virtual filesystem
~~~~~~~~~~~~~~~~~~

``TAISEI_RES_PATH``
   | Default: unset

   If set, overrides the default **resource directory** path. This is where Taisei looks for game data. The default path
   is platform and build specific:

   - On **macOS**, it will be the ``Contents/Resources/data`` directory inside of the ``Taisei.app`` bundle.
   - On **Linux**, **\*BSD**, and most other **Unix**-like systems (without ``-DRELATIVE``), it will be
     ``$prefix/share/taisei``. This path is static and determined at compile time.
   - On **Windows** and other platforms when built with ``-DRELATIVE``, it will be the ``data`` directory relative to
     the executable (or to whatever ``SDL_GetBasePath`` returns on the given platform).

``TAISEI_STORAGE_PATH``
   | Default: unset

   If set, overrides the default **storage directory** path. This is where Taisei saves your configuration, progress,
   screenshots and replays. Taisei also loads custom data from the ``resources`` subdirectory in there, if any, in
   addition to the stock assets. The custom resources shadow the default ones if the names clash. The default path is
   platform specific, and is equivalent to the return value of ``SDL_GetPrefPath("", "taisei")``:

   - On **Windows**, it’s ``%APPDATA%\taisei``.
   - On **macOS**, it’s ``$HOME/Library/Application Support/taisei``.
   - On **Linux**, **\*BSD**, and most other **Unix**-like systems, it’s ``$XDG_DATA_HOME/taisei`` or
     ``$HOME/.local/share/taisei``.

``TAISEI_CACHE_PATH``
   | Default: unset

   If set, overrides the default **cache directly** path. This is where Taisei stores transcoded Basis Universal
   textures and translated shaders to speed up future loading times.

   - On **Windows**, it’s ``%LOCALAPPDATA%\taisei\cache``.
   - On **macOS**, it’s ``$HOME/Library/Caches/taisei``.
   - On **Linux**, **\*BSD**, and most other **Unix**-like systems, it’s ``$XDG_CACHE_HOME/taisei`` or
     ``$HOME/.cache/taisei``.

   It is safe to delete this directly; Taisei will rebuild the cache as it loads resources. If you don’t want Taisei to
   write a persistent cache, you can set this to a non-writable directory.

Resources
~~~~~~~~~

``TAISEI_NOASYNC``
   | Default: ``0``

   If ``1``, disables asynchronous loading. Increases loading times, might slightly reduce CPU and memory usage during
   loads. Generally not recommended unless you encounter a race condition bug, in which case you should report it.

``TAISEI_NOUNLOAD``
   | Default: ``0``

   If ``1``, loaded resources are never unloaded. Increases memory usage, reduces filesystem reads and loading times
   over time.

``TAISEI_NOPRELOAD``
   | Default: ``0``

   If ``1``, disables preloading. All resources are only loaded as they are needed. Reduces loading times and memory
   usage, but may cause stuttering during gameplay.

``TAISEI_PRELOAD_REQUIRED``
   | Default: ``0``

   If ``1``, the game will crash with an error message when it attempts to use a resource that hasn’t been previously
   preloaded. Useful for developers to debug missing preloads.

``TAISEI_PRELOAD_SHADERS``
   | Default: ``0``

   If ``1``, Taisei will load all shader programs at startup. This is mainly useful for developers to quickly ensure
   that none of them fail to compile.

``TAISEI_AGGRESSIVE_PRELOAD``
   | Default: ``0``

   If ``1``, Taisei will scan all available resources and try to load all of them at startup. Implies
   ``TAISEI_NOUNLOAD=1``.

``TAISEI_BASISU_FORCE_UNCOMPRESSED``
   | Default: ``0``

   If ``1``, Basis Universal-compressed textures will be decompressed on the CPU and sent to the GPU in an uncompressed
   form. For debugging.

``TAISEI_BASISU_MAX_MIP_LEVELS``
   | Default: ``0``

   If ``>0``, limits the amount of mipmap layers loaded from Basis Universal textures. For debugging.

``TAISEI_BASISU_MIP_BIAS``
   | Default: ``0``

   If ``>0``, makes Taisei load lower resolution versions of Basis Universal textures that have mipmaps. Each level
   halves the resolution in each dimension.

``TAISEI_TASKMGR_NUM_THREADS``
   | Default: ``0`` (auto-detect)

   How many background worker threads to create for handling tasks such as resource loading. If ``0``, this will default
   to double the amount of logical CPU cores on the host machine.

Display and rendering
~~~~~~~~~~~~~~~~~~~~~

``TAISEI_VIDEO_RECREATE_ON_FULLSCREEN``
   | Default: ``0``; ``1`` on X11

   If ``1``, Taisei will re-create the window when switching between fullscreen and windowed modes. Can be useful to
   work around some window manager bugs.

``TAISEI_VIDEO_RECREATE_ON_RESIZE``
   | Default: ``0``; ``1`` on X11 and Emscripten

   If ``1``, Taisei will re-create the window when the window size is changed in the settings. Can be useful to work
   around some window manager bugs.

``TAISEI_RENDERER``
   | Default: ``gl33`` (but see below)

   Selects the rendering backend to use. Currently available options are:

   - ``gl33``: the OpenGL 3.3 Core renderer
   - ``gles30``: the OpenGL ES 3.0 renderer
   - ``sdlgpu``: the SDL3 GPU API renderer
   - ``null``: the no-op renderer (nothing is displayed)

   Note that the actual subset of usable backends, as well as the default choice, can be controlled by build options.
   The official releases of Taisei for Windows and macOS override the default to ``sdlgpu`` for improved compatibility.

``TAISEI_FRAMERATE_GRAPHS``
   | Default: ``0`` for release builds, ``1`` for debug builds

   If ``1``, framerate graphs will be drawn on the HUD.

``TAISEI_OBJPOOL_STATS``
   | Default: ``0``

   Displays some statistics about usage of in-game objects.

OpenGL and GLES renderers
~~~~~~~~~~~~~~~~~~~~~~~~~

``TAISEI_LIBGL``
   | Default: unset

   OpenGL library to load instead of the default. The value has a platform-specific meaning (it’s passed to the
   equivalent of ``dlopen``). Takes precedence over ``SDL_OPENGL_LIBRARY`` if set. Has no effect if Taisei is linked to
   libgl (which is not recommended, because it’s not portable).

``TAISEI_GL_DEBUG``
   | Default: ``0``

   Enables OpenGL debugging. A debug context will be requested, all OpenGL messages will be logged, and errors are
   fatal. Requires the ``KHR_debug`` or ``ARB_debug_output`` extension.

``TAISEI_GL_EXT_OVERRIDES``
   | Default: unset

   Space-separated list of OpenGL extensions that are assumed to be supported, even if the driver says they aren’t.
   Prefix an extension with ``-`` to invert this behaviour. Might be used to work around bugs in some
   weird/ancient/broken drivers, but your chances are slim. Note that this only affects code paths that actually test
   for the given extensions, not the actual OpenGL functionality. Some OpenGL implementations (such as Mesa) provide
   their own mechanisms for controlling extensions. You most likely want to use that instead.

``TAISEI_GL_WORKAROUND_DISABLE_NORM16``
   | Default: unset

   If ``1``, disables use of normalized 16 bit-per-channel textures in OpenGL. May be useful to work around broken
   drivers. If unset (default), will try to automatically disable them on drivers that are known to be problematic. If
   ``0``, 16-bit textures will always be used when available.

``TAISEI_GL33_CORE_PROFILE``
   | Default: ``1``

   If ``1``, try to create a Core profile context in the gl33 backend. If ``0``, create a Compatibility profile context.

``TAISEI_GL33_FORWARD_COMPATIBLE``
   | Default: ``1``

   If ``1``, try to create a forward-compatible context with some deprecated OpenGL features disabled.

``TAISEI_GL33_VERSION_MAJOR``
   | Default: ``3``

   Request an OpenGL context with this major version.

``TAISEI_GL33_VERSION_MINOR``
   | Default: ``3``

   Request an OpenGL context with this minor version.

``TAISEI_ANGLE_WEBGL``
   | Default: ``0``; ``1`` on Windows

   If ``1`` and the gles30 renderer backend has been configured to use ANGLE, it will create a WebGL-compatible context.
   This is needed to work around broken cubemaps in ANGLE’s D3D11 backend.

SDLGPU renderer
~~~~~~~~~~~~~~~

``TAISEI_SDLGPU_DEBUG``
   | Default: ``0``

   If ``1``, create the GPU context in debug mode. This enables some extra assertions in SDLGPU and a backend-specific
   debugging mechanism, if available On Vulkan this enables validation layers.

``TAISEI_SDLGPU_PREFER_LOWPOWER``
   | Default: ``0``

   If ``1``, prefer to select a low-power, efficient GPU for rendering when multiple are available. Usually this would
   be the integrated GPU on a laptop with both integrated and discrete graphics.

``TAISEI_SDLGPU_FAUX_BACKBUFFER``
   | Default: ``1``

   If ``1``, render the backbuffer into a staging texture before copying it to the swapchain at presentation. This is
   needed to emulate swapchain reads on SDLGPU, where the swapchain is write-only. Disabling this option eliminates the
   copy overhead, but breaks screenshots. If you don’t need the built-in screenshot functionality, it is safe to turn it
   off.

Audio
~~~~~

``TAISEI_AUDIO_BACKEND``
   | Default: ``sdl``

   Selects the audio playback backend to use. Currently available options are:

   - ``sdl``: the SDL2 audio subsystem, with a custom mixer
   - ``null``: no audio playback

   Note that the actual subset of usable backends, as well as the default choice, can be controlled by build options.

Timing
~~~~~~

``TAISEI_HIRES_TIMER``
   | Default: ``1``

   If ``1``, tries to use the system’s high resolution timer to limit the game’s framerate. Disabling this is not
   recommended; it will likely make Taisei run slower or faster than intended and the reported FPS will be less
   accurate.

``TAISEI_FRAMELIMITER_SLEEP``
   | Default: ``3``

   If over ``0``, tries to give up processing time to other applications while waiting for the next frame, if at least
   ``frame_time / this_value`` amount of time is remaining. Increasing this value reduces CPU usage, but may harm
   performance. Set to ``0`` for the v1.2 default behaviour.

``TAISEI_FRAMELIMITER_COMPENSATE``
   | Default: ``1``

   If ``1``, the framerate limiter may let frames finish earlier than normal after sudden frametime spikes. This
   achieves better timing accuracy, but may hurt fluidity if the framerate is too unstable.

``TAISEI_FRAMELIMITER_LOGIC_ONLY``
   | Default: ``0``
   | **Experimental**

   If ``1``, only the logic framerate will be capped; new rendering frames will be processed as quickly as possible,
   with no delay. This inherently desynchronizes logic and rendering frames, and therefore, some logic frames may be
   dropped if rendering is too slow. However, unlike with the synchronous mode, the game speed will remain roughly
   constant in those cases. ``TAISEI_FRAMELIMITER_SLEEP``, ``TAISEI_FRAMELIMITER_COMPENSATE``, and the ``frameskip``
   setting have no effect in this mode.

Demo Playback
~~~~~~~~~~~~~

``TAISEI_DEMO_TIME``
   | Default: ``3600`` (1 minute)

   How much time (in frames) of user inactivity is required to begin playing demo replays in the menu. If ``<=0``, demo
   playback will be disabled.


``TAISEI_DEMO_INTER_TIME``
   | Default: ``1800`` (30 seconds)

   How much time (in frames) of user inactivity is required to advance to the next demo in the sequence in between demo
   playback. This delay will be reset back to ``TAISEI_DEMO_TIME`` on user activity.

Kiosk Mode
~~~~~~~~~~

``TAISEI_KIOSK``
   | Default: ``0``

   If ``1``, run Taisei in “kiosk mode”. This forces the game into fullscreen, makes the window uncloseable, disables
   the “Quit” main menu option, and enables a watchdog that resets the game back to the main menu and default settings
   if there’s no user activity for too long.

   Useful for running a public “arcade cabinet” at events. You can customize the game’s default setting by placing a
   ``config.default`` file into one of the resource search paths, e.g. ``$HOME/.local/share/taisei/resources``. The
   format is the same as the ``config`` file created by Taisei in the storage directly.

``TAISEI_KIOSK_PREVENT_QUIT``
   | Default: ``0``

   If ``1``, allows users to quit the game in kiosk mode. Useful if you’re running a multi-game arcade cabinet setup.

``TAISEI_KIOSK_TIMEOUT``
   | Default: ``7200`` (2 minutes)

   Timeout for the reset watchdog in kiosk mode (in frames).

Frame Dump Mode
~~~~~~~~~~~~~~~

``TAISEI_FRAMEDUMP``
   | Default: unset
   | **Experimental**

   If set, enables the frame dump mode. In frame dump mode, Taisei will write every rendered frame as a ``.png`` file
   into a directory specified by this variable.

``TAISEI_FRAMEDUMP_SOURCE``
   | Default: ``screen``

   If set to ``screen``, the frame dump mode will record the whole window, similar to taking a screenshot. If set to
   ``viewport``, it will record only the contents of the in-game viewport framebuffer, and will only be active while
   in-game. Note that it is not the same as cropping a screenshot to the size of the viewport. Some elements that are
   rendered on top of the viewport, such as dialogue portraits, will not be captured.

``TAISEI_FRAMEDUMP_COMPRESSION``
   | Default: ``1``

   Level of deflate compression to apply to dumped frames, in the 0–9 range. Lower values will produce larger files that
   will encode faster. Larger values may create a large backlog of frames to encode that will consume a lot of RAM,
   depending on your CPU’s capabilities.

Miscellaneous
~~~~~~~~~~~~~

``TAISEI_GAMEMODE``
   | Default: ``1``
   | *Linux only*

   If ``1``, enables automatic integration with Feral Interactive’s GameMode daemon. Only meaningful for
   GameMode-enabled builds.

``TAISEI_REPLAY_DESYNC_CHECK_FREQUENCY``
   | Default: ``300``

   How frequently to write desync detection hashes into replays (every X frames). Lowering this value results in larger
   replays with more accurate desync detection. Intended for debugging desyncing replays with ``--rereplay``.

Logging
~~~~~~~

Taisei’s logging system currently has five basic levels and works by dispatching messages to a few output handlers. Each
handler has a level filter, which is configured by a separate environment variable. All of those variables work the same
way: their value looks like an IRC mode string, and represents a modification of the handler’s default settings. If this
doesn’t make sense, take a look at the *Examples* section.

The levels
^^^^^^^^^^

- **Debug** (*d*) is the most verbose level. It contains random information about internal workings of the game and is
  disabled for release builds at source level.
- **Info** (*i*) logs some events that are expected to occur during normal operation, for example when a spell is
  unlocked or a screenshot is taken.
- **Warning** (*w*) usually complains about misuse of the engine features, deprecations, unimplemented functionality,
  other small anomalies that aren’t directly detrimental to functionality.
- **Error** (*e*) alerts of non-critical errors, for example a missing optional resource, corrupted progress data, or
  failure to save a replay due to insufficient storage space or privileges.
- **Fatal** (*f*) is an irrecoverable failure condition. Such an event most likely signifies a programming error or a
  broken installation. The game will immediately crash after writing a message with this log level. On some platforms,
  it will also display a graphical message box.
- **All** (*a*) is not a real log level, but a shortcut directive representing all possible log levels. See *Examples*
  for usage.

The variables
^^^^^^^^^^^^^

``TAISEI_LOGLVLS_CONSOLE``
   | Default: ``+a`` *(All)*

   Controls what log levels may go to the console. This acts as a master switch for ``TAISEI_LOGLVLS_STDOUT`` and
   ``TAISEI_LOGLVLS_STDERR``.

``TAISEI_LOGLVLS_STDOUT``
   | Default: ``+di`` *(Debug, Info)*

   Controls what log levels go to standard output. Log levels that are disabled by ``TAISEI_LOGLVLS_CONSOLE`` are
   ignored.

``TAISEI_LOGLVLS_STDERR``
   | Default: ``+wef`` *(Warning, Error, Fatal)*

   Controls what log levels go to standard error. Log levels that are disabled by ``TAISEI_LOGLVLS_CONSOLE`` are
   ignored.

``TAISEI_LOGLVLS_FILE``
   | Default: ``+a`` *(All)*

   Controls what log levels go to the log file (``{storage directory}/log.txt``).

``TAISEI_LOG_ASYNC``
   | Default: ``1``

   If ``1``, log messages are written asynchronously from a background thread. This mostly benefits platforms where
   writing to the console or files is very slow (such as Windows). You may want to disable this when debugging.

``TAISEI_LOG_ASYNC_FAST_SHUTDOWN``
   | Default: ``0``

   If ``1``, don’t wait for the whole log queue to be written when shutting down. This will make the game quit faster if
   log writing is slow, at the expense of log integrity. Ignored if ``TAISEI_LOG_ASYNC`` is disabled.

``TAISEI_SDL_LOG``
   | Default: ``0``

   If ``>0``, redirects SDL’s log output into the Taisei log. The value controls the minimum log priority; see
   ``SDL_log.h`` for details.

Examples
^^^^^^^^

- In release builds: print *Info* messages to stdout, in addition to *Warning*\ s, *Error*\ s, and *Fatal*\ s as per
  default:

   .. code:: sh

      TAISEI_LOGLVLS_STDOUT=+i

- In Debug builds: remove *Debug* and *Info* output from the console:

   .. code:: sh

      TAISEI_LOGLVLS_STDOUT=-di

   OR:

   .. code:: sh

      TAISEI_LOGLVLS_CONSOLE=-di

- Don’t save anything to the log file:

   .. code:: sh

      TAISEI_LOGLVLS_FILE=-a

- Don’t print anything to the console:

   .. code:: sh

      TAISEI_LOGLVLS_CONSOLE=-a

- Don’t save anything to the log file, except for *Error*\ s and *Fatal*\ s:

   .. code:: sh

      TAISEI_LOGLVLS_FILE=-a+ef

- Print everything except *Debug* to ``stderr``, nothing to ``stdout``:

   .. code:: sh

      TAISEI_LOGLVLS_STDOUT=-a
      TAISEI_LOGLVLS_STDERR=+a-d
