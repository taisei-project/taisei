Complex Numbers in Taisei
=========================

Introduction
''''''''''''

Taisei uses [complex numbers](https://en.wikipedia.org/wiki/Complex_number) in
its game code for a number of different functions, such as:

* Player and enemy position and movement
* Danmaku patterns and bullet movement
* Special effects and particles
* Background drawing

As you can probably see, it's important to have a firm grasp of complex numbers
in order to make heads or tails of what the game is doing if you want to develop
for it.

Complex numbers shouldn't be scary. If you have a baseline of middle school
mathematics, it should only take you a couple of hours to get used to thinking
in these terms, and using them effectively in the code.

If you're the type of person that does better with video/audio than text, here
are some very educational (and short!) videos on imaginary numbers:

* [Imaginary Numbers are Real](https://www.youtube.com/watch?v=T647CGsuOVU&list=PLiaHhY2iBX9g6KIvZ_703G3KJXapKkNaF)
  by Welch Labs - short and punchy video series
* [Complex Number Fundamentals](https://www.youtube.com/watch?v=5PcpBw5Hbwo) by
  3Blue1Brown - a longer lecture-style format

Important Stuff
'''''''''''''''

(Stuff goes here.)

In-Game Examples
''''''''''''''''

Simple Movement
^^^^^^^^^^^^^^^

Lets take a few concrete examples within Taisei's code. For let's look at a
piece of movement code for a fairy:

```c
    boss->move = move_towards(VIEWPORT_W/2.0 + 200.0 * I, 0.035);
```

The important piece here, for our purposes, is:

```c
    VIEWPORT_W/2.0 + 200.0 * I
```

What this is doing is specifying a position on the X-axis (``VIEWPORT_W/2.0``,
which in Taisei means `480 / 2.0` or `240`), and then specifying a position on
the "imaginary" (aka "lateral") axis (``200.0 * I``). The ``I`` here is the same
``i`` described in the videos and the above explanation.

So what we're really looking at here is:

```c
    240.0 (real) + 200.0i (imaginary)
```

Or "move 240 units on the X-axis, and then 200 units on the Y-axis (called the
'imaginary axis' in complex vectors)."

This is what's called a **Cartesian Coordinate.** What the function
`move_towards` then does is make the enemy sprite/object move towards that point
on the X/Y axis at a certain rate (defined by `0.035`).

Simple Danmaku
^^^^^^^^^^^^^^

Of course, there'd be no point in using complex numbers over a simpler X/Y
system if it didn't provide significant advantages. So let's look at a more
complicated danmaku pattern to see why complex numbers are more effective for
this role.

```c
    cmplx aim = cnormalize(global.plr.pos - enemy->pos);
```

This `aim` variable could be passed to a `move_towards` function attached
to a `PROJECTILE` object. The effect here would be the bullets moving directly
towards the player in a straight line, even if the player moves around the
screen, following the player as they move around in real-time (assuming) more
than one bullet is fired.

Let's look at the argument inside `cnormalize` first, `global.plr.pos - e->pos`.
Both `global.plr.pos` and `e->pos` are *complex numbers*, in that they have both
*real* and *imaginary* parts. Much like the example in ``Simple Movement``, they
represent a place on the X/Y grid.

In the format of `[X,Y]`, let's say that `global.plr.pos` is `[-1, 6]`, and that
`enemy->pos` is `[6, 3]`.

#INSERT IMAGE HERE

When you subtract `[6, 3]` (enemy position) from `[-1, 6]` (player position),
you end up with `[-7, 3]`, as seen here with `plr->pos`.

#INSERT IMAGE HERE

This also conveniently lets the enemy position `enemy->pos` become the new
"origin," or `[0, 0]`. This is useful because it means that we can more easily
determine what angle the danmaku need to travel in to travel towards the player.

`cnormalize()` in the original function runs this new resulting "player
position minus enemy position" complex number through a kind of "smoothing"
function and returns the angle between `[0, 0]` and `[-7, 3]`. You can see the
formula used for converting a *cartesean coordinate* to a *polar coordinate*
[here](https://www.engineeringtoolbox.com/converting-cartesian-polar-coordinates-d_1347.html),
but for the sake of brevity, the result here is **157°**, which you can see
from the orange line rotating counter-clockwise to about 157°.

And so, the danmaku will move approximately 157° from the X/Y origin... and that
happens to be where the player is! This is all done in real-time using complex
numbers to make it easier to calculate procedurally, and is used in many other
2D games.

Further Reading
'''''''''''''''

A more advanced form of these numbers, called [quaternions](https://en.wikipedia.org/wiki/Quaternion),
is used in 3D space as well, so if you have a strong desire to get into game
design, complex numbers are a good place to start, since you only have to think
about it in terms of 2D.
