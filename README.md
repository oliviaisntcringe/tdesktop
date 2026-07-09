# tuerlegram
<img width="1536" height="1024" alt="image" src="https://github.com/user-attachments/assets/6163e69a-578f-4b9d-918a-0859e8f4b5f3" />

**tuerlegram** is a custom [Telegram Desktop][telegram_desktop] fork with an amber-CRT terminal look and a set of power-user / privacy features. It is built on the [Telegram API][telegram_api] and the [MTProto][telegram_proto] secure protocol.

[![Build Status](https://github.com/oliviaisntcringe/tdesktop/workflows/MacOS./badge.svg)](https://github.com/oliviaisntcringe/tdesktop/actions)
[![Build Status](https://github.com/oliviaisntcringe/tdesktop/workflows/Linux./badge.svg)](https://github.com/oliviaisntcringe/tdesktop/actions)

The source code is published under GPLv3 with OpenSSL exception, the license is available [here][license].

## What's different from upstream

tuerlegram keeps everything Telegram Desktop does and adds:

* **Amber CRT reskin** — a warm amber-on-black terminal theme applied by default, a global monospace font, square avatars, sharp message bubbles, amber panel divider frames, and an F-key status bar at the bottom of the window.
* **Terminal boot sequence** — a short fake boot log on startup.
* **Dumps** — a Settings section that exports whole chats to CSV and keeps a managed library of past dumps; also a "Dump dialog" chat context-menu action.
* **Loud alert** — a per-chat toggle that fires an impossible-to-miss alert (window flash + modal box) on every new post in a flagged channel.
* **Save one-time media** — save view-once / self-destruct photos and videos.
* **Custom Touch Bar chats** (macOS) — choose exactly which chats appear on the Touch Bar instead of the fixed pinned list.

This is a personal fork for the author's own use. It is not affiliated with or endorsed by Telegram.

## Build instructions

Building matches upstream Telegram Desktop:

* [Windows (32-bit and 64-bit)][win]
* [macOS][mac]
* [GNU/Linux using Docker][linux]

You will need your own `api_id` / `api_hash` — see [docs/api_credentials.md](docs/api_credentials.md).

## Third-party

* Qt 6 ([LGPL](http://doc.qt.io/qt-6/lgpl.html)) and Qt 5.15 ([LGPL](http://doc.qt.io/qt-5/lgpl.html)) slightly patched
* OpenSSL 3.2.1 ([Apache License 2.0](https://openssl-library.org/source/license/apache-license-2.0.txt))
* WebRTC ([New BSD License](https://github.com/desktop-app/tg_owt/blob/master/LICENSE))
* zlib ([zlib License](http://www.zlib.net/zlib_license.html))
* LZMA SDK 9.20 ([public domain](http://www.7-zip.org/sdk.html))
* liblzma ([public domain](http://tukaani.org/xz/))
* Google Breakpad ([License](https://chromium.googlesource.com/breakpad/breakpad/+/master/LICENSE))
* Google Crashpad ([Apache License 2.0](https://chromium.googlesource.com/crashpad/crashpad/+/master/LICENSE))
* GYP ([BSD License](https://github.com/bnoordhuis/gyp/blob/master/LICENSE))
* Ninja ([Apache License 2.0](https://github.com/ninja-build/ninja/blob/master/COPYING))
* OpenAL Soft ([LGPL](https://github.com/kcat/openal-soft/blob/master/COPYING))
* Opus codec ([BSD License](http://www.opus-codec.org/license/))
* FFmpeg ([LGPL](https://www.ffmpeg.org/legal.html))
* Guideline Support Library ([MIT License](https://github.com/Microsoft/GSL/blob/master/LICENSE))
* Range-v3 ([Boost License](https://github.com/ericniebler/range-v3/blob/master/LICENSE.txt))
* Open Sans font ([Apache License 2.0](http://www.apache.org/licenses/LICENSE-2.0.html))
* Vazirmatn font ([SIL Open Font License 1.1](https://github.com/rastikerdar/vazirmatn/blob/master/OFL.txt))
* Emoji alpha codes ([MIT License](https://github.com/emojione/emojione/blob/master/extras/alpha-codes/LICENSE.md))
* xxHash ([BSD License](https://github.com/Cyan4973/xxHash/blob/dev/LICENSE))
* QR Code generator ([MIT License](https://github.com/nayuki/QR-Code-generator#license))
* CMake ([New BSD License](https://github.com/Kitware/CMake/blob/master/Copyright.txt))
* Hunspell ([LGPL](https://github.com/hunspell/hunspell/blob/master/COPYING.LESSER))
* Ada ([Apache License 2.0](https://github.com/ada-url/ada/blob/main/LICENSE-APACHE))

Based on [Telegram Desktop][telegram_desktop] by Telegram FZ-LLC.

[//]: # (LINKS)
[telegram_desktop]: https://desktop.telegram.org
[telegram_api]: https://core.telegram.org
[telegram_proto]: https://core.telegram.org/mtproto
[license]: LICENSE
[win]: docs/building-win.md
[mac]: docs/building-mac.md
[linux]: docs/building-linux.md
