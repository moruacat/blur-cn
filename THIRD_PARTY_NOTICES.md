# Third-Party Notices

This repository is a Simplified Chinese (zh-CN) localization fork of
[blur](https://github.com/f0e/blur). The original project is licensed under the
GNU General Public License v3.0; see `LICENSE`.

## HarmonyOS Sans

The Simplified Chinese user interface **falls back** to the **HarmonyOS Sans SC**
font on systems where the Windows system font is not available.

- Files bundled in this repository:
  - `resources/fonts/HarmonyOS_Sans_SC_Medium.ttf` (unmodified original file)
  - `resources/fonts/LICENSE.txt` (original license text)
- Copyright 2021 Huawei Device Co., Ltd.
- Licensed under the *HarmonyOS Sans Fonts License Agreement*. The full text is
  included in `resources/fonts/LICENSE.txt` and is also installed next to the
  application in the `fonts` directory.
- The font file is distributed **unmodified**. It has not been subsetted,
  renamed, or otherwise altered.
- The font is bundled with the application solely so that the localized user
  interface can render Chinese text.

## Microsoft YaHei UI

On Windows the localized interface prefers the **Microsoft YaHei UI** font that
ships with the operating system (`%WINDIR%\Fonts\msyh.ttc`). This font is loaded
at runtime and is **not** redistributed with this repository or its installers;
it remains covered by the Microsoft Windows license terms.

## Other bundled components

Other third-party libraries (Dear ImGui, stb, SDL3, FFmpeg, VapourSynth plugins,
etc.) are covered by their respective licenses as described by the upstream
[blur](https://github.com/f0e/blur) project.
