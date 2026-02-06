泰西Project - 环境变量
======================================

.. contents::

介绍
------------

泰西的某些更高级配置选项由 `环境变量
<https://zh.wikipedia.org/wiki/环境变量>`__ 控制。 本文档试图描述这些选项。

通常情况下，你无需设置任何环境变量。这些变量仅供开发者和高级用户使用。因此，我们假设您已经熟悉这些概念。

除了此处列出的变量外，（除非另有说明，）游戏的运行时依赖项（例如 SDL3）处理的环境变量也会生效，


变量
---------

虚拟文件系统
~~~~~~~~~~~~~~~~~~

``TAISEI_RES_PATH``
   | 默认：未设置

   如果设置，则覆盖默认的 **资源目录** 路径。泰西会在此查找游戏数据。默认路径取决于平台和构建版本：

   - 在 **macOS** 上, 它应该在 ``Taisei.app`` 压缩包里的 ``Contents/Resources/data`` 目录。
   - 在 **GNU/Linux**, **\*BSD**, 以及其他类 **Unix** 系统 (不带 ``-DRELATIVE`` 参数), 它应该在
     ``$prefix/share/taisei`` 。该路径是静态的，在编译时确定。
   - 在 **Windows** 和其他构建时带有 ``-DRELATIVE`` 参数的平台, 它应该在可执行文件的目录下的 ``data`` 文件夹 (或者指向特定平台上 ``SDL_GetBasePath`` 函数给所返回的路径。）

``TAISEI_STORAGE_PATH``
   | 默认：未设置

   如果设置，则覆盖默认的 **存储目录** 路径。Taisei 会在此保存您的配置、进度、屏幕截图和录像。泰西还会从其中的 ``resources`` 子目录中加载自定义数据（如果有），除了库存资源之外。如果名称冲突，自定义资源会覆盖默认资源。默认路径因平台而异，与  ``SDL_GetPrefPath("", "taisei")`` 的返回值有关：

   - 在 **Windows**, 它位于 ``%APPDATA%\taisei``.
   - 在 **macOS**, 它位于 ``$HOME/Library/Application Support/taisei``.
   - 在 **GNU/Linux**, **\*BSD**, 以及其他类 **Unix** 系统, 它位于 ``$XDG_DATA_HOME/taisei`` 或者
     ``$HOME/.local/share/taisei``.

``TAISEI_CACHE_PATH``
   | 默认：未设置

   如果设置，则覆盖默认的 **直接缓存** 路径。泰西将在此存储转码后的 Basis Universal 纹理和编译后的着色器，以加快未来的加载速度。

   - 在 **Windows**, 它位于 ``%LOCALAPPDATA%\taisei\cache``.
   - 在 **macOS**, 它位于 ``$HOME/Library/Caches/taisei``.
   - 在 **GNU/Linux**, **\*BSD**, 以及其他类 **Unix** 系统, 它位于 ``$XDG_CACHE_HOME/taisei`` 或者
     ``$HOME/.cache/taisei``.

   直接删除此目录没问题；泰西会在加载资源时重建缓存。如果你不希望缓存，可以直接将其设置为不可写目录。

资源
~~~~~~~~~

``TAISEI_NOASYNC``
   | 默认： ``0``

   如果设置为 ``1`` ，则禁用异步加载。这会增加加载时间，但可能会略微降低加载期间的 CPU 和内存使用率。通常不建议使用此选项，除非遇到一些错误（如果遇到这种情况，去开个issue）

``TAISEI_NOUNLOAD``
   | 默认： ``0``

   如果设置为 ``1`` ，则已加载的资源永远不会从内存删除。随着时间的推移，内存使用量会增加，文件系统读取和加载时间也会减少。

``TAISEI_NOPRELOAD``
   | 默认： ``0``

   如果设置为 ``1`` ，则禁用预加载，所有资源仅在需要时加载。这可以减少加载时间和内存占用，但可能会导致游戏卡顿。

``TAISEI_PRELOAD_REQUIRED``
   | 默认： ``0``

   如果设置为 ``1`` ，游戏在尝试使用之前未预加载的资源时会崩溃并显示错误消息。（这是用来调试的）

``TAISEI_PRELOAD_SHADERS``
   | 默认： ``0``

   如果设置为 ``1`` ，泰西将在启动时加载所有着色器程序。这主要有助于开发者顺利编译程序。

``TAISEI_AGGRESSIVE_PRELOAD``
   | 默认： ``0``

   如果设置为 ``1`` ，泰西将扫描所有可用资源，并尝试在启动时加载所有资源。同时也会启用 ``TAISEI_NOUNLOAD=1`` 。

``TAISEI_BASISU_FORCE_UNCOMPRESSED``
   | 默认： ``0``

   如果设置为 ``1`` ，Basis Universal压缩纹理将在CPU上解压缩，并以未压缩的形式发送到GPU。（用于调试）

``TAISEI_BASISU_MAX_MIP_LEVELS``
   | 默认： ``0``

   如果设置大于 ``0``, 将限制从Basis Universal纹理加载的mipmap层数。（用于调试）

``TAISEI_BASISU_MIP_BIAS``
   | 默认： ``0``

   如果设置大于 ``0``, 泰西会加载具有mipmap的Basis Universal纹理的低分辨率版本。每调高1，就将分辨率减半。

``TAISEI_TASKMGR_NUM_THREADS``
   | 默认： ``0`` (自动检测)

   处理资源加载等任务而创建的后台工作线程数量。如果设置为 ``0`` ，则默认为逻辑CPU核心数量的两倍。

显示和渲染
~~~~~~~~~~~~~~~~~~~~~

``TAISEI_VIDEO_RECREATE_ON_FULLSCREEN``
   | 默认： ``0``; X11上是 ``1`` 

   如果设置为 ``1`` ，泰西将在全屏和窗口模式之间切换时重新创建窗口。这有助于解决一些窗口管理器的错误。

``TAISEI_VIDEO_RECREATE_ON_RESIZE``
   | 默认： ``0``; X11与Emscripten上是 ``1``

   如果设置为“1”，泰西将在设置中更改窗口大小时重新创建窗口。这有助于解决某些窗口管理器的错误。

``TAISEI_RENDERER``
   | 默认： ``gl33`` (但请看下文)

   选择要使用的渲染后端。当前可用的选项有：

   - ``gl33``: OpenGL 3.3 核心渲染器
   - ``gles30``: OpenGL ES 3.0渲染器
   - ``sdlgpu``: SDL3 GPU API渲染器
   - ``null``: 无操作渲染器（不显示任何内容）

   请注意，实际可用后端以及默认选项可以通过构建选项进行控制。
   泰西的 Windows 和 macOS 官方版本将默认值覆盖为 ``sdlgpu`` ，以提高兼容性。

``TAISEI_FRAMERATE_GRAPHS``
   | 默认： ``0`` （release构建）, ``1`` （debug构建）

   如果设置为 ``1``, 会显示帧率图表。

``TAISEI_OBJPOOL_STATS``
   | 默认： ``0``

   显示一些有关游戏内物体使用情况的统计数据。

OpenGL和GLES渲染器
~~~~~~~~~~~~~~~~~~~~~~~~~

``TAISEI_LIBGL``
   | 默认：未设置

   要加载的OpenGL库，将替代默认库使用。该值的含义与平台相关（会传递至等效于 ``dlopen`` 的函数）。
   若已设置，将优先于 ``SDL_OPENGL_LIBRARY`` 生效。若泰西已链接至libgl则此设置无效（不建议这样做，因为会丧失跨平台性）。

``TAISEI_GL_DEBUG``
   | 默认： ``0``

   启用OpenGL调试功能。将请求调试上下文，记录所有OpenGL消息，并使所有错误成为致命错误。
   需要 ``KHR_debug`` 或 ``ARB_debug_output`` 扩展。

``TAISEI_GL_EXT_OVERRIDES``
   | 默认：未设置

   假装设置的OpenGL扩展受支持 (即使驱动程序报告不支持）(各扩展间用空格分隔)。在扩展名前添加 ``-`` 可反向操作。
   可用于规避某些怪异/古老/有问题的驱动程序中的错误（但成功几率渺茫）。请注意，这仅影响实际检测给定扩展的代码路径，而非实际的OpenGL功能。
   某些OpenGL实现（如Mesa）提供了自有扩展控制机制，建议优先使用那些机制。

``TAISEI_GL_WORKAROUND_DISABLE_NORM16``
   | 默认：未设置

   若设置为 ``1`` ，则禁用OpenGL中规范化16位每通道纹理的使用。可用于规避存在缺陷的驱动程序问题。
   未设置时（默认值），将在已知有问题的驱动程序上自动尝试禁用此功能。若设为 ``0`` ，则只要可用就会始终使用16位纹理。

``TAISEI_GL33_CORE_PROFILE``
   | 默认： ``1``

   若设为 ``1`` ，尝试在gl33后端创建核心配置文件上下文。
   若设为 ``0`` ，则创建兼容性配置文件上下文。

``TAISEI_GL33_FORWARD_COMPATIBLE``
   | 默认： ``1``

   若设为 ``1`` ，则尝试创建前向兼容的上下文，同时禁用某些已弃用的OpenGL功能。

``TAISEI_GL33_VERSION_MAJOR``
   | 默认： ``3``

   请求此主版本（OpenGL 4.6前面那个4）的OpenGL上下文。

``TAISEI_GL33_VERSION_MINOR``
   | 默认： ``3``

   请求此小版本（OpenGL 4.6后面那个6）的OpenGL上下文。

``TAISEI_ANGLE_WEBGL``
   | 默认： ``0``

   若设为 ``1`` 且gles30渲染器后端已配置使用ANGLE时，将创建与WebGL兼容的上下文。
   此设置用于规避ANGLE的D3D11后端中存在缺陷的立方体贴图功能。

SDLGPU渲染器
~~~~~~~~~~~~~~~

``TAISEI_SDLGPU_DEBUG``
   | 默认： ``0``

   若设为 ``1`` ，将在调试模式下创建GPU上下文。这将启用SDLGPU中的额外断言机制，并在可用时启用后端特定的调试功能。在Vulkan后端中此设置将启用验证层。

``TAISEI_SDLGPU_PREFER_LOWPOWER``
   | 默认： ``0``

   若设为 ``1`` ，当存在多个GPU时，将优先选择低功耗的GPU进行渲染。在同时配备集成显卡和独立显卡的笔记本电脑上，通常此选项会选择集成显卡。

``TAISEI_SDLGPU_FAUX_BACKBUFFER``
   | 默认： ``1``

   若设为 ``1`` ，将在呈现前将后备缓冲区渲染到暂存纹理中，再复制到交换链。此操作用于在SDLGPU（其交换链为只写模式）上模拟交换链读取功能。
   禁用此选项可消除复制开销，但会导致截图功能失效。如果不需要的话就关掉吧。

音频
~~~~~

``TAISEI_AUDIO_BACKEND``
   | 默认： ``sdl``

   选择要使用的音频播放后端。当前可用选项有：

   - ``sdl`` ：使用SDL2音频子系统及自定义混音器
   - ``null`` ：无音频播放功能

   请注意，实际可用的后端以及默认选择可通过构建选项进行控制。

计时
~~~~~~

``TAISEI_HIRES_TIMER``
   | 默认： ``1``

   若设为 ``1`` ，将尝试使用系统高精度计时器来限制游戏帧率。
   不建议禁用此选项，否则可能导致游戏运行速度偏离预期（过快或过慢），且报告的帧率数值准确度会下降。

``TAISEI_FRAMELIMITER_SLEEP``
   | 默认： ``3``

   若设置值大于 ``0`` ，在等待下一帧时若剩余时间至少为``帧时间/此设定值``，将尝试将处理时间让给其他应用程序。
   提高该值可降低CPU使用率，但可能影响性能。设为 ``0`` 可恢复v1.2版本的默认行为。

``TAISEI_FRAMELIMITER_COMPENSATE``
   | 默认： ``1``

   若设为 ``1`` ，帧率限制器在出现突发性帧时间飙升后，可能让帧提前结束渲染。
   这种方式能实现更精确的时序控制，但当帧率过于不稳定时可能会影响画面流畅度。

``TAISEI_FRAMELIMITER_LOGIC_ONLY``
   | 默认： ``0``
   | **实验性功能**

   若设为 ``1`` ，将仅限制逻辑帧率；渲染帧会以最快速度处理而无延迟。这将导致逻辑帧与渲染帧不同步，因此当渲染速度过慢时可能会丢弃部分逻辑帧。
   但与同步模式不同，在此情况下游戏速度仍能保持大致稳定。
   ``TAISEI_FRAMELIMITER_SLEEP`` 、 ``TAISEI_FRAMELIMITER_COMPENSATE`` 及 ``跳帧`` 设置在此模式下无效。

Demo回放
~~~~~~~~~~~~~

``TAISEI_DEMO_TIME``
   | 默认： ``3600`` (1分钟)

   菜单中开始播放Demo回放所需用户无操作的时间（按帧数计算）。若设置为小于等于 ``0`` ，将禁用Demo回放功能。


``TAISEI_DEMO_INTER_TIME``
   | 默认： ``1800`` (30秒)

   在演示回放间隙中，切换到序列中下一个演示所需用户无操作的时间（按帧数计算）。用户有操作时将把此延迟重置回 ``TAISEI_DEMO_TIME`` 的值。

Kiosk模式
~~~~~~~~~~

``TAISEI_KIOSK``
   | 默认： ``0``

   若设为 ``1`` ，将以「kiosk模式」运行泰西。此模式会强制游戏全屏运行，使窗口不可关闭，禁用主菜单中的"退出"选项，并启用看门狗机制——当长时间无用户操作时将自动重置游戏至主菜单并恢复默认设置。

   这是为公开展示泰西设置的类街机模式。可通过在资源搜索路径（如 ``$HOME/.local/share/taisei/resources`` ）中放置 ``config.default`` 文件来自定义游戏默认设置，其格式与泰西在存储目录创建的 ``config`` 文件相同。

``TAISEI_KIOSK_PREVENT_QUIT``
   | 默认： ``0``

   若设为 ``1`` ，将允许用户在kiosk模式下退出游戏。适用于运行多游戏街机系统的场景。

``TAISEI_KIOSK_TIMEOUT``
   | 默认： ``7200`` 

   kiosk模式下重置看门狗的超时时间（按帧数计算）。

帧转储模式
~~~~~~~~~~~~~~~

``TAISEI_FRAMEDUMP``
   | 默认：未设置
   | **实验性功能**

   如果设置，则启用帧转储模式。在帧转储模式下，泰西会将每个渲染帧以 ``.png`` 文件的形式写入此变量指定的目录中。

``TAISEI_FRAMEDUMP_SOURCE``
   | 默认： ``screen``

   若设置为 ``screen`` ，帧转储模式将录制整个窗口（类似于截图功能）。若设置为 ``viewport`` ，则仅录制游戏内视口帧缓冲区的内容，且仅在游戏过程中激活。
   请注意：这与将截图裁剪至视口尺寸不同，某些渲染在视口顶部的元素（如对话立绘）将不会被捕获。

``TAISEI_FRAMEDUMP_COMPRESSION``
   | 默认： ``1``

   应用于转储帧的deflate压缩级别（0-9范围）。
   数值越低会生成文件体积更大但编码速度更快的文件；数值越高可能因CPU处理能力不足而造成帧编码积压，从而消耗大量内存。

其他
~~~~~~~~~~~~~

``TAISEI_GAMEMODE``
   | 默认： ``1``
   | *仅在GNU/Linux可用*

   若设为 ``1`` ，将启用与Feral Interactive的GameMode守护进程的自动集成功能。仅对支持GameMode的编译版本有效。

``TAISEI_REPLAY_DESYNC_CHECK_FREQUENCY``
   | 默认： ``300``

   向回放文件中写入同步检测哈希值的频率（每X帧写入一次）。降低此值会使回放文件体积增大，但能提高同步检测的精确度。该功能主要用于通过 ``--rereplay`` 参数调试回放同步问题。

日志
~~~~~~~

泰西的日志系统目前有五个基本级别，其工作原理是将消息分派到几个输出处理程序。每个处理程序都有一个级别过滤器，该过滤器由单独的环境变量配置。所有这些变量的工作方式相同：它们的值类似于 IRC 模式字符串，表示对处理程序默认设置的修改。如果你对此不理解，请见下文。

级别
^^^^^^^^^^

- **debug**  ( *d* )：最详细的日志级别。包含游戏内部运行的各种信息，在Release版本中已在源码级别禁用此级别。
- **info**  ( *i*  )：记录正常运行期间预期发生的事件，例如解锁符卡或截取屏幕截图时。
- **warning**  ( *w* )：通常针对引擎功能误用、已弃用功能、未实现功能以及其他不影响核心功能的轻微异常情况发出提示。
- **error**  ( *e* )：报告非致命性错误，例如缺失可选资源、进度数据损坏、或因存储空间或权限不足导致回放保存失败。
- **fatal**  ( *f* )：表示不可恢复的故障状态。此类事件通常意味着程序错误或安装损坏。游戏在记录该级别消息后将立即崩溃。部分平台还会显示图形化消息框。
- **all**  ( *a* )：并非真实日志级别，而是代表所有可能日志级别的快捷指令。用法参见*示例*部分。

变量
^^^^^^^^^^^^^

``TAISEI_LOGLVLS_CONSOLE``
   | 默认： ``+a`` *(All)*

   控制哪些日志级别可以发送到控制台。相当于 ``TAISEI_LOGLVLS_STDOUT`` 和
   ``TAISEI_LOGLVLS_STDERR``.

``TAISEI_LOGLVLS_STDOUT``
   | 默认： ``+di`` *(Debug, Info)*

   控制哪些日志级别会被输出到标准输出流。被 ``TAISEI_LOGLVLS_CONSOLE`` 禁用的日志级别将被忽略。

``TAISEI_LOGLVLS_STDERR``
   | 默认： ``+wef`` *(Warning, Error, Fatal)*

   控制哪些日志级别输出到标准错误流。被 ``TAISEI_LOGLVLS_CONSOLE`` 禁用的日志级别将不会输出。

``TAISEI_LOGLVLS_FILE``
   | 默认： ``+a`` *(All)*

   控制哪些日志级别输出到日志文件（``{存储目录}/log.txt``）。

``TAISEI_LOG_ASYNC``
   | 默认： ``1``

   若设为 ``1`` ，将通过后台线程异步写入日志消息。这对控制台或文件写入速度较慢的平台（如Windows）尤其有益。调试时建议禁用此选项。

``TAISEI_LOG_ASYNC_FAST_SHUTDOWN``
   | 默认： ``0``

   若设为 ``1`` ，关闭游戏时不等待完整日志队列写入完毕。当日志写入速度较慢时，此设置可加速游戏退出进程，但会牺牲日志完整性。
   若 ``TAISEI_LOG_ASYNC`` 被禁用则此设置无效。

``TAISEI_SDL_LOG``
   | 默认： ``0``

   若设置为 ``0`` ，将SDL的日志输出重定向至泰西日志系统。该数值控制最低日志优先级，详情参见 ``SDL_log.h`` 。

例子
^^^^^^^^

- 在Release构建: 从标准输出流输出 *Info* 信息, 此外按照 *Warning*\ , *Error*\  , 和 *Fatal*\   排序。
  默认：

   .. code:: sh

      TAISEI_LOGLVLS_STDOUT=+i

- 在Debug构建: 从控制台删掉 *Debug* 和 *Info* 输出:

   .. code:: sh

      TAISEI_LOGLVLS_STDOUT=-di

   或者:

   .. code:: sh

      TAISEI_LOGLVLS_CONSOLE=-di

- 不要保存任何日志:

   .. code:: sh

      TAISEI_LOGLVLS_FILE=-a

- 不要在我控制台上输出:

   .. code:: sh

      TAISEI_LOGLVLS_CONSOLE=-a

- 不要保存任何日志, 除了 *Error*\  和 *Fatal*\ :

   .. code:: sh

      TAISEI_LOGLVLS_FILE=-a+ef

- 将除 *Debug* 之外的所有内容输出到 ``stderr``，不输出到 ``stdout``:

   .. code:: sh

      TAISEI_LOGLVLS_STDOUT=-a
      TAISEI_LOGLVLS_STDERR=+a-d
