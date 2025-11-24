泰西Project
==============

.. image:: misc/icons/taisei.ico
   :width: 150
   :alt: Taisei Project icon

.. contents::

介绍
------------

关于泰西Project
^^^^^^^^^^^^^^^^^^^^

《泰西Project》是一个自由开源的 `东方Project
<https://zh.wikipedia.org/wiki/东方Project>`__ 同人游戏。 这是一款竖版弹幕射击游戏（STG）,
STG是节奏明快的游戏，注重模式识别并通过反复练习来掌握技巧。

泰西Project具备高可移植性, 以C11规范编写, 使用SDL3和OpenGL渲染器。官方支持
Windows, GNU/Linux, macOS, 以及带有WebGL的浏览器（Firefox与基于Chromium的浏览器(Chrome, Edge），它也可以编译到其他操作系统里。

想要看实机截图, 可以去 `我们的网站 <https://taisei-project.org/media>`__.

需要游戏介绍, 可以读一下 `这个 <doc/GAME_CN.rst>`__.

想要了解剧情, 可以读一下 `这个 <doc/STORY_CN.txt>`__. (小心剧透！)

关于东方Project
^^^^^^^^^^^^^^^^^^

东方Project是一个独立游戏系列 (或者说“同人”) ，以其丰富的角色阵容和令人难忘的配乐而闻名。 它基本上是由一位名叫ZUN的作者创作，
并具有 `宽松的版权许可 <https://thwiki.cc/东方Project使用规定案>`__  ，这使得泰西Project等同人作品能够合法存在。

泰西 *并非* 只是东方的复制品, 它有自己的音乐、美术、游戏机制和源代码以及原创故事。 
虽然对东方Project有一定的了解会有所帮助，但即使不了解，也可以享受到本游戏的乐趣。


欲了解更多同人文化， `点这里 <https://zh.wikipedia.org/wiki/同人文化>`__.

安装
------------

你可以在Github上的 `Releases
<https://github.com/taisei-project/taisei/releases>`__ 界面获取编译好的二进制文件, 有Windows (x64), GNU/Linux, 和macOS的可用文件。

还有适用于 Nintendo Switch（自制软件）的实验版本（风险自负）。

你甚至还可以 `在这 <https://play.taisei-project.org/>`__ 玩实验性的WebGL版本。
(支持基于Chromium的浏览器和Firefox)

源代码与开发
-------------------------

获取源代码
^^^^^^^^^^^^^^^^^^^^^

Git
______

建议使用 ``git`` 获取源代码:

.. code:: sh

   git clone --recurse-submodules https://github.com/taisei-project/taisei

每当拉入新代码、检查另一个分支或执行任何 ``git`` 操作时，你也可以运行 ``git submodule update`` 。  ``./pull`` 和 ``./checkout`` 帮助脚本可以自动完成此操作。

存档
_______

⚠️ **注意**: 由于 GitHub 打包源代码的方式，主仓库上的“Download ZIP”链接 *不起作用* ,
这是因为 GitHub 在自动生成 ``.zip`` 文件时不会将子模块与源代码一起打包。
所以你应该去 `Releases <https://github.com/taisei-project/taisei/releases>`__ 界面下载。

编译源代码
^^^^^^^^^^^^^^^^^^^^^

目前，我们建议在类POSIX系统（GNU/Linux、macOS等）上构建泰西。

虽然泰西具有高度可配置性，但编译代码的最简单方法是：

.. code:: sh

   meson setup build/
   meson compile -C build/
   meson install -C build/

阅读 `构建 <./doc/BUILD.rst>`__ 文档以了解构建泰西的更多信息与依赖。

Replay，截图以及配置文件的位置
--------------------------------------------

泰西将所有数据存储在特定于平台的目录中：

- 在 **Windows**, 它们应该在 ``%APPDATA%\taisei``
- 在 **macOS**, 它们应该在 ``$HOME/Library/Application Support/taisei``
- 在 **GNU/Linux**, **\*BSD**, 以及其他类 **Unix** 系统, 它们应该在 ``$XDG_DATA_HOME/taisei`` 或者
  ``$HOME/.local/share/taisei``

这被称为 **存储目录** ，可以设置环境变量 ``TAISEI_STORAGE_PATH`` 来覆盖。

故障排除
---------------

在我们的 `文档章节 <./doc/README_CN.rst>`__ 可以找到很多有价值的信息。

如果您在编译或运行泰西时遇到任何问题，请去 `打开一个issue <https://github.com/taisei-project/taisei/issues>`__.

联系我们
-------

- https://taisei-project.org/
- `我们的Discord服务器 <https://discord.gg/JEHCMzW>`__
