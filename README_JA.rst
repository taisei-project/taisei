泰西Project
===========

.. image:: misc/icons/taisei.ico
   :width: 150
   :alt: Taisei Project icon

.. contents::

はじめに
--------

泰西Project について
^^^^^^^^^^^^^^^^^^^^

泰西Project は、\ `東方Project <https://ja.wikipedia.org/wiki/東方Project>`__\ の世界を舞台にした、オープンソースのファンゲーム（二次創作ゲーム）です。見下ろし視点の縦スクロール弾幕シューティングゲーム（STG）で、弾幕のパターンを見切り、練習を重ねて上達していく、テンポの速いゲームです。

泰西Project は移植性が高く、C11 で書かれ、SDL3 と OpenGL レンダラーを使っています。Windows、Linux、macOS、そして WebGL に対応したブラウザ（Firefox や、Chrome・Edge などの Chromium 系ブラウザ）を公式にサポートしています。ほかの多くの OS 向けにもコンパイルできます。

プレイ画面のスクリーンショットは\ `公式サイト <https://taisei-project.org/media>`__\ をご覧ください。

遊び方は\ `こちら <doc/GAME_JA.rst>`__\ を読んでください。

ストーリーは\ `こちら <doc/STORY_JA.txt>`__\ を読んでください（ネタバレ注意！）。

東方Project について
^^^^^^^^^^^^^^^^^^^^

東方Project は、個性豊かな多数のキャラクターと印象的な音楽で知られる、同人ゲームのシリーズです。ほぼ ZUN 氏ひとりの手で制作されており、\ `二次創作に寛容なガイドライン <https://touhou-project.news/guideline/>`__\ があるおかげで、泰西Project のような二次創作作品が正当に存在できます。

泰西Project は東方の「クローン」\ *ではありません*\ 。独自の音楽、絵、ゲームシステム、コードベースで、オリジナルの物語を描いています。東方を知っていればより楽しめますが、シリーズの予備知識がなくても、単体で遊べます。

同人文化について詳しくは\ `こちら <https://ja.wikipedia.org/wiki/同人>`__\ をご覧ください。

インストール
------------

ビルド済みの完全版は、GitHub の\ `Releases <https://github.com/taisei-project/taisei/releases>`__\ ページから入手できます。Windows（x64）、Linux、macOS 向けがあります。

Nintendo Switch（homebrew）向けの実験的なビルドもあります（利用は自己責任でお願いします）。

実験的な WebGL 版を、ウェブブラウザから\ `こちら <https://play.taisei-project.org/>`__\ で遊ぶこともできます（Chromium 系ブラウザと Firefox に対応）。

ソースコードと開発
------------------

ソースコードの入手
^^^^^^^^^^^^^^^^^^

ソース
______

ソースコードは ``git`` で取得することをおすすめします。

.. code:: sh

   git clone --recurse-submodules https://github.com/taisei-project/taisei

新しいコードを pull したとき、別のブランチを checkout したとき、そのほか ``git`` の操作をしたときは、そのつど ``git submodule update`` も実行してください。補助スクリプトの ``./pull`` と ``./checkout`` を使えば、自動で行われます。

アーカイブ
__________

⚠️ **注意**：GitHub がソースコードをまとめる仕組みのため、リポジトリのトップにある ``Download ZIP`` のリンクは\ *使えません*\ 。

GitHub が ``.zip`` ファイルを自動生成するとき、サブモジュールを一緒に含めないためです。代わりにこちらで手作業でアーカイブを作っているので、\ **必ず**\ `Releases <https://github.com/taisei-project/taisei/releases>`__\ ページからダウンロードしてください。

ソースコードのコンパイル
^^^^^^^^^^^^^^^^^^^^^^^^

現時点では、POSIX 系のシステム（Linux、macOS など）でのビルドをおすすめします。

泰西Project は細かく設定できますが、お使いのマシン向けにコンパイルするいちばん簡単な方法は次のとおりです。

.. code:: sh

   meson setup build/
   meson compile -C build/
   meson install -C build/

ビルド方法の詳細と依存関係の一覧は、\ `Building <./doc/BUILD.rst>`__\ （英語）をご覧ください。

リプレイ、スクリーンショット、設定の保存場所
--------------------------------------------

泰西Project は、すべてのデータをプラットフォームごとのディレクトリに保存します。

- **Windows** では、おそらく ``%APPDATA%\taisei``
- **macOS** では ``$HOME/Library/Application Support/taisei``
- **Linux**\ 、\ **\*BSD**\ 、そのほか多くの **Unix** 系システムでは、\ ``$XDG_DATA_HOME/taisei`` または ``$HOME/.local/share/taisei``

これを\ **ストレージディレクトリ**\ と呼びます。環境変数 ``TAISEI_STORAGE_PATH`` を設定すると、場所を変更できます。

トラブルシューティング
----------------------

開発やゲームコントローラーへの対応など、多くの話題についての文書が\ `ドキュメントのページ <./doc/README.rst>`__\ （英語）にあります。

コンパイルや実行で問題が起きたら、遠慮なく\ `Issue を立てて <https://github.com/taisei-project/taisei/issues>`__\ ください。

連絡先
------

- https://taisei-project.org/
- `Discord のサーバー <https://discord.gg/JEHCMzW>`__
