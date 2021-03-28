Graphical Assets
================

.. contents::

Introduction
------------

Taisei's GFX library is made up of a collection of sprites, shaders, and a few
3D models. The 3D models are almost exclusively used for background scenery
(and a few other places, like the HUD), while the sprites are used in everything
from UI elements, character portraits, to the
danmaku bullets themselves.

To modify the 3D models, you'll need `Blender <https://blender.org>`__, which is
free and open source. Look for tutorials on YouTube for how to use it. The
models themselves are located in ``resources/00-taisei.pkgdir/models``.

Taisei uses ``.obj`` for its 3D models. To export ``.obj`` files from Blender,
use ``File -> Export -> Wavefront (.obj)``. Ensure that the following settings
are used:

::

    Include
        Objects as OBJ Objects: ENABLED

    Transform
        Forward: Y Forward
        Up: -Z Up

    Geometry
        Write Materials: DISABLED
        Triangulate Faces: ENABLED

Music and sound effects are located in ``resources/00-taisei.pkgdir/sfx``.

For sprites, any image editor will do. Sprites are located in ``atlas``.
However, to have sprites properly appear in Taisei, you'll need a few packages
and tools first to rebuild the atlas so the game can load them properly.

You'll need ``rectpack`` and ``pillow`` from ``Python PIP``:

.. code:: sh

    pip3 install rectpack pillow

You'll also need to download (and/or compile) and install
`Leanify <https://github.com/JayXon/Leanify>`__.

You'll need to run one of the following commands to regenerate the ``atlas``
once the sprites have been modified. Pay attention to which directory you've
made your changes in (such as ``common_ui``) and use the appropriate command.`

.. code:: sh

    ninja gen-atlas-common_ui
    ninja gen-atlas-common
    ninja gen-atlas-portraits

Or, to regenerate *everything*:

.. code:: sh

    ninja gen-atlases

That will regenerate the files needed for your new sprites to appear correctly.

*Generally speaking*, Taisei prefers ``.webp`` as the final product, but can
convert ``.png`` into ``.webp`` using the above ``ninja gen-atlas*`` commands.

