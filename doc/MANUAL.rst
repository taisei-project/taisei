.. _taisei-project--game-manual:

Taisei Project – Game Manual
============================

.. contents::

Introduction
------------

Taisei is a 2D vertical-scrolling shooting game (STG), also known as a "bullet
hell". You play as one of several characters and progress through six different
stages of increasing difficulty, shooting bullets at your opponents and dodging
their bullets in return.

If you get hit even once, you lose a life; if you are out of lives, it’s Game
Over, but you get a few Continues to keep playing (without scores). This
difficulty is balanced by the fact that your hitbox is very small, and you're
very fast, as well as having a limited number of "bombs" to temporarily clear
the screen of bullets if you get cornered.

It is a game of precision, concentration, memorization, and practice. Most
players take many tries to get to the end of the game, slowly making progress
over time as they learn the game and come to recognize the patterns, as well as
developing their own strategies for certain parts of the game. A full "run" of
the game only takes about 30 minutes, so you're encouraged to just keep trying
over and over again until you get that sought-after "1cc" (1 credit clear),
where you don't use a Continue.

Difficulty
----------

Taisei is a very hard game, especially for newcomers to the genre. In many
modern games, *Easy* is a placeholder and *Normal* is the easy mode so people
playing it don't feel inadequate with themselves, so you might feel inclined to
start at Normal like in your other games.

This approach doesn’t work for Taisei though. Easy is balanced around not being
impossible for newcomers, but it’ll still require some trial and error.  On the
positive side it still contains enough bullets to show you the beauty of
_danmaku_.

There is no shame in playing Easy. Some say that even the dev who initially
founded the project and wrote this section can’t beat Easy…

(We're also working on balancing Easy/Normal.)

Characters and Shotmodes
------------------------

The three different playable characters have different shots. You start out
weak and have to raise your power meter to get stronger.

Try a few modes to find the one that fits you best.

HUD
---

Once you reach the game itself, a bunch of numbers will appear on the right
of your screen (HUD). These are:

* **Score** Your current score. See `Scoring System`_, grazing, and collecting
  items.
* **Lives** How many lives you have. The *Next* value shows the next score at
  which you receive a bonus new life.
* **Spell Cards** How many `Bombs`_ you have. If your *Fragment* count goes to
  500, you get a new bomb.
* **Power** How strong your current shot is. Maxes out at 4, but can be
  overcollected until 6.
* **Value** `Point Item Value`_ at the top of the screen.
* **Volts** See `Scoring System`_.
* **Graze** Raise it by getting close to bullets.

Controls
--------

+-------------+------------------------------------------+
| Name        | Default Key                              |
+=============+==========================================+
| Shoot       | ``Z``                                    |
+-------------+------------------------------------------+
| Bomb        | ``X``                                    |
+-------------+------------------------------------------+
| Power Surge | ``C``                                    |
+-------------+------------------------------------------+
| Focus       | ``Left Shift``                           |
+-------------+------------------------------------------+
| Move        | ``Up``,\ ``Down``,\ ``Left``,\ ``Right`` |
+-------------+------------------------------------------+

If you don’t like the defaults, you can easily remap them in the settings.

Using the Gamepad is also supported. See Options to set up controls.

Users of non-qwerty keyboard layouts: Don’t worry, Taisei’s controls are
not based on layout but key location.

Focus
-----

While pressing ``Shift``, your movement is slower and the pattern of your
shot changes. It is useful for accurate dodging, but you shouldn’t stay
in this mode all the time.

The white circle appearing in this mode is a representation of your
hitbox. As you can see it is really small.

Bombs (Player Spell Cards)
--------------------------

In a stressful life where all the bullets kill you in a single hit, you
sometimes want something powerful to shoot back with. Thankfully, your
character has magical power to trigger so called Spell Cards. These are
essentially bombs that do a lot of damage.

You can use them to clear out bullets and enemies from the screen.  However you
only get a limited amount of them, so they are best used when you are in a
pinch. When used against a boss' Spell Card, it voids your capturing bonus.

Rumor has it that if you get hit by a bullet and hit the bomb key fast enough,
you can avoid death. This is called *Death Bombing* and people who master it --
so the lore goes -- will find their bomb meter becoming a second life meter.

Power Surge
-----------

You can activate a special aura that gives you increased damage and a way to
clear a limited area of bullets. Read more about it in `Scoring System`_.

Items
-----

Now that you can kill enemies you will notice that they drop various amounts of
items. They may look like bullets at first, but they are safe to touch.

These are:

* **Blue** Point items that increase your score.
* **Red** Gives you more power. Comes in different sizes, always with a capital
  P.
* **Green Star** Bomb. Appears either filled (full bomb) or as an empty outline
  (worth 500 bomb fragments).
* **Pink Heart** One extra life.
* **PIV Items** Small yellow ghosts that increase the value of your point items.
* **Surge Lightning** Will spawn during your Power Surge to provide charge.
* **Voltage** Spawn after a Power Surge completes and increase PIV (*a lot*),
  Bombs and Volts.

*An important tip:* if you fly near to the top of the screen, all the visible
items will be picked up (shown as flying towards you).

Bosses
------

Taisei has 6 levels (called *stages*). Each stage has a boss and a midboss in
some form. They are much stronger than normal enemies and have different
attacks with time limits. There are different types of Attacks:

- **Normal**: A signature move every boss has. They are a break between the
  other, more fierce attacks, but don’t let your guard down.

- **Spell Card**: This is where the Bosses concentrate their powers (resulting
  in a background change) and hit you with really hard and unusual patterns.
  They give a lot of extra points and 100 bomb fragments when you *capture*
  them. That means shooting down the HP within the time limit without getting
  hit or using bombs.

  You can revisit spellcards you have encountered in the *Spell Practice* mode
  to get better at the ones you frequently die on.

- **Voltage Overdrive**: Collect enough `Voltage`_ to unlock these at the end
  of the boss battles.

  These are tricky unique spells that will take some creativity to dodge.  Due
  to the extremely ionized Danmaku conditions, your Bomb and Life meters are
  malfunctioning. You can’t be hurt, but you can’t use your bombs either.

  One boss seems to be especially attuned to these surroundings and awaits you
  with about the strangest spell in the game. If you capture it, it may unlock
  something nice.

- **Survival Spell**: Rarely, a very strong boss can invoke a Spell Card that
  makes them completely invincible. You are on your own here. Try to survive
  somehow until the timer runs out.

  You might want to use *Spell Practice* to perfect one of them.

Scoring System
--------------

Scoring might seem like something important for the adept pro player only. The
lowly easy mode player just cares about surviving, right? Not necessarily!  In
Taisei, you are rewarded with extra lives as you score. So while the statement
from the beginning is true to an extent, knowing the basics of getting a good
score (and the non score-related benefits you get along the way) is helpful for
everyone.

Point Item Value
""""""""""""""""

The amount of score you collect is not a flat value. It depends on different
factors you can influence to maximize the amount of points you earn. Point
items for example give more score if they are collected higher up on the
screen. If you go up beyond a certain point, the game will also auto collect
all items on the screen.

Auto collection is also triggered by other events such as bombs, and the items
collected in this way will always count as collected at the top, so it is
beneficial for your score.

The point items themselves also get more valuable as you cancel or graze
bullets at full power among other things.

Voltage
"""""""

The most visible part of the scoring system is the Power Surge mechanic.  While
Power 4.00 is the maximum your shots will put out, you can overcharge your
Power meter up to 6.00. The surplus Power (and also the rest, if you are in a
bind) can be used to start a Power Surge (see `Controls`_).

The Power Surge will charge up the air around you with Danmaku electricity.
This boosts your damage by 20%, but you have to maintain the two kinds of
charges in the charge meter around your character. When the positive (yellow)
charge goes down and hits the negative, the surge ends. The special lightning
items that appear during the surge replenish both kinds of charges. By timing
these items you can control the relative amounts of charge and the higher your
negative charge, the more difficult the surge becomes to keep up.

The longer you continue this game and the more damage and negative charge you
stack up, the more powerful your surge will become. This will visibly increase
the radius of your sparkly aura.  Once the surge ends, all of it is released in
a blast that damages enemies and bullets. Be in the right spot when that
happens, because all that havoc will be transformed into collectible Voltage
items.

These will increase your Point Item Value, give you 1 Bomb Fragment and also
add to your Volts meter.

If your Voltage reaches the Breakdown level shown in the HUD, you unlock a
special spell at the end of the stage.

More info
---------

Knowing this much should help to get you started!

If you want more tricks and hints on how to *“git gud”*, check out resources on
how to play *Tōhō*, the game Taisei is based on.

Enjoy playing, and if you want to contact us, visit us on Freenode IRC
#taisei-project or on `Discord <https://discord.gg/JEHCMzW>`__.
