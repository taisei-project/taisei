.. _taisei-project--game-manual:

.. role:: strike
   :class: strike

Taisei Project - Game Manual
============================

.. contents::

Introduction
------------

Taisei is a shoot-em-up game. Lots of bullets cover the screen and if you are hit by one, you die. To make things
easier, your hitbox is very small—much smaller than your character.

If you get hit you lose a life; if you are out of lives, it’s Game Over, but you get a few Continues to keep playing
(without scores).

Difficulty
----------

Taisei is a very hard game, especially for newcomers to the genre. In many modern games, *Easy* is a placeholder and
*Normal* is the easy mode so people playing it don’t have to feel bad, so you might feel inclined to start at Normal
like in your other games.

This approach doesn’t work for Taisei though. Easy is balanced around maybe being not impossible for newcomers, so it’ll
require some training. On the positive side it still contains enough bullets to show you the beauty of all the patterns.
;)

There is no shame in playing Easy. Some say that even the dev who initially founded the project and wrote this section
can’t beat Easy…

Characters and Shotmodes
------------------------

The three different playable characters have different shots. You start out weak and have to raise your power meter to
get stronger.

Try a few modes to find the one that fits you best.

HUD
---

Once you reach the game itself, a bunch of numbers will appear on the right of your screen (HUD). These are:

- **Score** Your current score. See `Scoring System`_.
- **Lives** How many lives you have. The *Next* value shows the next score at which you receive a bonus new life.
- **Spell Cards** How many `Bombs`_ you have. If your *Fragment* count goes to 500, you get a new bomb.
- **Power** How strong your current shot is. Maxes out at 4, but can be overcollected until 6.
- **Value** `Point Item Value`_ at the top of the screen.
- **Volts** See `Scoring System`_.
- **Graze** Raise it by getting close to bullets.

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

Using the Gamepad is also supported. See options to set up controls.

Users of non-QWERTY keyboard layouts: Don’t worry, Taisei’s controls are not based on layout but key location.

Focus
-----

While pressing ``Shift``, your movement is slower and the pattern of your shot changes. It is useful for accurate
dodging, but you shouldn’t stay in this mode all the time.

The white circle appearing in this mode is a representation of your hitbox. As you can see it is really small.

.. _Bombs:

Bombs (Player Spell Cards)
--------------------------

In a stressful life where all the bullets kill you in a single hit, you sometimes want something powerful to shoot back
with. Thankfully, your character has magical power to trigger so called Spell Cards. These are essentially bombs that do
a lot of damage.

You can use them to clear out bullets and enemies from the screen. However you only get a limited amount of them, so
they are best used when you are in a pinch. When used against a boss’ Spell Card, it voids your capturing bonus.

Rumor has it that if you get hit by a bullet and hit the bomb key fast enough, you can avoid death. This is called
*Death Bombing* and people who master it—so the lore goes—will find their bomb meter becoming a second life meter.

Power Surge
-----------

Taisei has a unique mechanic called power surge, activated and discharged using the C key by default and requiring 2.00 power for every activation. By default, it will also activate automatically once the player reaches 6.00 power. Though activating a surge costs 2.00 power to use, the player’s number of orbs/options/familiars remains the same until the surge is completed. On activation, the surge HUD appears and all items on-screen are collected.

During a surge, player damage is increased by 20%, point items collected at any location are at full PIV and graze is changed from spawning small power or PIV items to spawning surge items. These and power items serve to build charge, indicated by the blue and yellow arcs in the HUD. Both types, the stable blue negative and more easily fluctuating yellow positive charges are increased in the ways mentioned above and decrease passively over time.

The surge will continue until the positive (orange) and negative (blue) charges displayed around the character are equal. Both types of charges drain at different rates but can be increased by collecting power items or the new lightning items that appear from grazing bullets. The more blue charge accumulates, the harder it becomes to keep the surge going. The accumulated charge also affects the growth of the surge’s area of effect, damage, and point value on discharge.

When a surge is discharged, whether manually, by pressing the activation key or automatically through getting hit or charge neutralization, all bullets inside the area of effect are canceled and turned into voltage items. Enemies in the same radius are damaged, also producing voltage items. If the discharge was caused by anything other than the player’s being hit, the voltage items it creates will be autocollected. Voltage items dramatically increase point item value and will add to the volt meter in the HUD, which can be used to unlock a secret Overdrive Spell in each stage.

Items
-----

Now that you can kill enemies you will notice that they drop various amounts of items. They may look like bullets at
first, but they are safe to touch.

These are:

- **Blue** Point items that increase your score.
- **Red** Gives you more power. Comes in different sizes, always with a capital P.
- **Green Star** Bomb. Appears either filled (full bomb) or as an empty outline (worth 500 bomb fragments).
- **Pink Heart** One extra life.
- **PIV Items** Small yellow ghosts that increase the value of your point items.
- **Surge Lightning** Will spawn during your Power Surge to provide charge.
- **Voltage** Spawn after a Power Surge completes and increase PIV (*a lot*), Bombs and Volts.

If you fly near to the top of the screen, all the visible items will be picked up (shown as flying towards you).

Bosses
------

Taisei has 6 levels (called *stages*). Each stage has a boss and a midboss in some form. They are much stronger than
normal enemies and have different attacks with time limits. There are different types of Attacks:

- **Normal**: A signature move every boss has. They are a break between the other, more fierce attacks, but don’t let
  your guard down.

- **Spell Card**: This is where the Bosses concentrate their powers (resulting in a background change) and hit you with
  really hard and unusual patterns. They give a lot of extra points and 100 bomb fragments when you *capture* them. That
  means shooting down the HP within the time limit without getting hit or using bombs.

  You can revisit spellcards you have encountered in the *Spell Practice* mode to get better at the ones you frequently
  die on.

- **Voltage Overdrive**: Collect enough Voltage to unlock these at the end of the boss battles.

  These are tricky unique spells that will take some creativity to dodge. Due to the extremely ionized Danmaku
  conditions, your Bomb and Life meters are malfunctioning. You can’t be hurt, but you can’t use your bombs either.

  One boss seems to be especially attuned to these surroundings and awaits you with about the strangest spell in the
  game. If you capture it, it may unlock something nice.

- **Survival Spell**: Rarely, a very strong boss can invoke a Spell Card that makes them completely invincible. You are
  on your own here. Try to survive somehow until the timer runs out.

  You might want to use *Spell Practice* to perfect one of them.

Scoring System
--------------

Scoring might seem like something important for the adept pro player only. The lowly easy mode player just cares about
surviving, right? Not necessarily! In Taisei, you are rewarded with extra lives as you score. So while the statement
from the beginning is true to an extent, knowing the basics of getting a good score (and the non score-related benefits
you get along the way) is helpful for everyone.

.. _Point Item Value:
**Point Item Value**
    The amount of score you collect is not a flat value. It depends on different factors you can influence.

    Point items for example give more score if they are collected higher up on the screen.
    The maximum value of a point item (collected at the top of the screen) is referred to as Point Item Value, or     PIV. It’s displayed on the HUD next to the
    blue point icon. It can be increased by collecting small Value items that usually spawn when bullets are canceled, grazing above 4.00 power, or, most efficiently, using the Power Surge system.

    PIV also affects some other values, such as spell card bonuses.

**Autocollection**
    In various situations, the player will automatically collect all items on the screen. Each item collected in this way counts as collected at the top of the screen, which is beneficial for scoring.

    Autocollection happens in the following situations:

    * The player moves above a certain height on the screen
    * Bombs
    * Power Surges

**Power Surges**
    Power Surges are the most important aspect of the scoring system in Taisei as they increase PIV, give you extra resources, trigger autocollection, cancel tough patterns without failing spells, and unlock the extra overdrive spells to collect even more score.

**Bonuses**
    These come from clearing stages and capturing spells and make up most of your score, with the most common and significant variable being your PIV.

More info
---------

Knowing this much should help to get you started!

If you want more tricks and hints on how to *“git gud”*, check out resources on how to play *Tōhō*, the game Taisei is
based on.

Enjoy playing, and if you want to contact us, visit us on `Discord
<https://discord.gg/JEHCMzW>`__.
