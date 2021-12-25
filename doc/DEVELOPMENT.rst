Taisei Project - Development FAQ
================================

.. contents::

Introduction
------------

Some development tips for working with Taisei's code.

Coding Style
------------

Upkeep Script
"""""""""""""

``ninja`` has a function called ``upkeep`` which uses scripts found in
``taisei/scripts/upkeep/`` to check against common issues such as poor RNG use,
deprecation warnings, and other helpful code health checks. You don't have to
run it constantly, but once in a while will make sure that basic things are
more readily caught.

It requires a setup ``build/`` directory, so make sure you've completed the basic
``meson`` setup instructions seen on the `main README </README.rst#compiling-source-code>`__.

.. code:: sh

   # from the root taisei/ directory
   ninja upkeep -C build/

Tabs/Spaces
"""""""""""

In the ``*.c`` files, tabs are used. In the ``meson.build`` and ``*.py`` files,
spaces are used. It's a bit inconsistent, but it's the style that was chosen at
the beginning, and one we're probably going to stick with.

To help you abide by this standard, you should install
`EditorConfig <https://github.com/editorconfig>`__ for your preferred editor of
choice, and load in the file found at ``.editorconfig`` in the root of the
project.

For Loops
"""""""""

In general, things like ``for`` loops should have no spaces between the ``for`` and opening brace (``(``). For example:

.. code:: c

   # incorrect
   for (int i = 0; i < 10; i++) { log_debug(i); }

   # correct
   for(int i = 0; i < 10; i++) { log_debug(i); }

``ctags`` Support
'''''''''''''''''

A ``tags`` file can assist certain code editors (like ``vim``) with jump-to-definition support and other useful features. To generate a ``taisei/tags`` file.

.. code:: sh

   ninja ctags -C build/

You then have to let your editor know that a ``tags`` file exists.

Using ``.vimrc`` as an example:

.. code:: sh

   # this will walk the project directory until it finds a .ctagsdb file
   set tags=tags;

Compiling Issues
----------------

-Wunused-variable
"""""""""""""""""

If you get an error compiling your code, but you're 100% sure that you've
actually used the variable, chances are you're using that variable in an
``assert()`` and are compiling with ``clang``.

``clang`` won't recognize that the variable is actually being used in an
``assert()``.

You can use the macro ``attr_unused`` to bypass that warning. This:

.. code:: c

   int x = 0;
   assert(x == 0);

Becomes this:

.. code:: c

   attr_unused int x = 0;
   assert(x == 0);

Platform-Specific Tips
----------------------

macOS
"""""

Tools
'''''

On macOS, you need to begin with installing the Xcode Command Line Tools:

.. code:: sh

   xcode-select --install

For other tools, such as ``meson``, you can acquire those by using
`Homebrew <https://brew.sh/>`__.

Libraries
'''''''''

As of 2021-08-05, you should **not** install the following packages via
Homebrew, as the versions available do not compile against Taisei correctly.
If you're having mysterious errors, ensure that they're not installed.

* ``spirv-tools``
* ``spirv-cross``
* ``sdl2_mixer``

Remove them with:

.. code:: sh

   brew remove spirv-tools spirv-cross sdl2_mixer

Taisei-compatible versions are bundled and will be pulled in at compile time.

In addition, if you're trying to compile on an older version of macOS
(i.e: <10.12), SDL2 may not compile correctly on Homebrew (as of 2019-02-19).
Let ``meson`` pull in the corrected version for you via subprojects.

**NOTE:** While Homebrew's optional dependencies greatly improve compile times,
if you can't remove packages that give you errors from your system for whatever
reason, you can force ``meson`` to use its built-in subprojects by using the
following option:

.. code:: sh

   meson configure build/ --wrap-mode=forcefallback

``ctags`` Error
'''''''''''''''

You may run into an error when generating ``.ctagsdb`` file, such as ``illegal option -- L`` or something similar. This is because the version of ``ctags`` that ships with Xcode isn't directly supported by ``ninja``.

You can install and alias the Homebrew version of ``ctags`` by doing the following:

.. code:: sh

   brew install ctags
   # either run this in console, or add to your shell profile
   alias ctags='/usr/local/bin/ctags'

Live Resource Reloading (macOS)
'''''''''''''''''''''''''''''''

The required library for live resource reloading of shaders, etc., known as
``libinotify``, is available natively on Linux, but macOS requires a
third-party driver to gain ths same functionality.

In order to use live resource reloading on macOS, you'll need to download,
build, and install
`libinotify-kqueue <https://github.com/libinotify-kqueue/libinotify-kqueue>`__
onto your system.

Once that's done, you'll also need to boost the limit on open file handlers.
This is due to an OS-level limit on how many files an application can open at a
single time, and Taisei keeps many files open for monitoring with
live-reloading enabled.

⚠️ **In summary, if you run into an error of ``Too many open files``,** you'll
need to fix it using this command:

.. code:: sh
   # '1024' is a reasonable boost from macOS's default of `256`
   # but it can also be any number you want
   ulimit -n 1024

This is not a system-wide or permanent setting, and will need to be executed on
every shell you start. The simplest way to do this is to add the above command
to your ``~/.zshrc`` or ``~/.bashrc`` depending on what shell you use. (The
default on modern macOS is ``zsh``.) You can check the status of the limit with:

.. code:: sh
   # this should output `1024` if everything worked
   ulimit -n
   > 1024

