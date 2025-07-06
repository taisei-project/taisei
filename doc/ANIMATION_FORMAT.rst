The Animation Sequence Format
=============================

An animation is made up of a number of sprites and an ``.ani`` file which contains all the metadata.

The ``.ani`` file needs to specify the number of sprites using the ``@sprite_count`` attribute. Then different animation
sequences can be defined.

Animation sequences are chains of sprites that can be replayed in-game. For example, Cirno can either fly normally or
flex while flying. In order for the game to understand which sprites need to be shown in what order and time delay, you
need to define a sequence for every action.

To define the action *right* of the player flying to the left for example, you write into the file

.. code:: c

   right = d5 0 1 2 3

Every key in the .ani file not starting with ``@`` corresponds to a sequence. The sequence specification itself is a
list of frame indices. In the example above, the right sequence will cycle frames 0–3. Everything that is not a number
like ``d5`` in the example is a parameter:

``d<n>``
   sets the frame delay to ``n``. This means every sprite index given is shown for ``n`` in-game frames.
``m``
   toggles the mirroring of the following frames.
``m0,m1``
   set the absolute mirroring of the following frames.

All parameters are persistent within one sequence specification until you change them.

More examples:

.. code:: c

   flap = d5 0 1 2 3 d2 4 5 6 7
   left = m d5 0 1 2 3
   alternateleftright = m d5 0 1 2 3 m 0 1 2 3

There are many possibilities to use ``d<n>`` to make animations look dynamic (or strange)

Some naming conventions
^^^^^^^^^^^^^^^^^^^^^^^

The resource code does not require you to choose any specific names for your sequences, but different parts of the game
of course have to at some point. The most common convention is calling the standard sequence “main”. This is the least
you need to do for anything using an aniplayer, because the aniplayer needs to know a valid starting animation.

If you have a sequence of the sprite going left or right, call it “left” and “right”. Player and fairy animations do
this.

Look at existing files for more examples. Wriggle might be interesting for complicated delay and queue trickery. The
documentation of the ``AniPlayer`` struct might also be interesting to learn more about the internals.
