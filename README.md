# Blur

<p align="center">
  <img src="https://github.com/user-attachments/assets/adc158b9-8ec4-4e5a-b372-ed1fad9d8d61" width="30%" />
  <img src="https://github.com/user-attachments/assets/eebbac0d-6fa1-42ed-beeb-e85ea93838b6" width="30%" />
  <img src="https://github.com/user-attachments/assets/e8b749dc-9232-4e45-93b4-8df2e3854ffb" width="30%" />
</p>

[![Downloads](https://img.shields.io/github/downloads/f0e/blur/total?label=Downloads)](https://github.com/f0e/blur/releases/latest) [![Discord](https://img.shields.io/discord/1392389164153962640?style=flat&label=Discord)](https://discord.gg/B5BK9GMN87)

Blur is a native desktop application made for easily and efficiently adding motion blur to videos through frame blending, with the ability to utilise frame interpolation and more.

Join the [Discord](https://discord.gg/B5BK9GMN87) to share your configs, render tests and ask the community for help.

## 简体中文界面

这是 [f0e/blur](https://github.com/f0e/blur) 的简体中文汉化分支。汉化范围仅限图形界面（设置项、按钮、通知和提示）；日志、报错信息和 `--help` 等文本保持英文，方便搜索和向上游反馈。

汉化版通过仓库自带的 GitHub Actions 工作流构建：推送到自己的 fork 后，在对应 Actions 运行的 **Artifacts** 中下载 `Blur-Release`，其中包含 Windows 安装包 `blur-Windows-Installer-x64.exe`。

### 中文字体

界面优先使用 Windows 自带的 **Microsoft YaHei UI**（`%WINDIR%\Fonts\msyh.ttc`，即资源管理器、记事本和 ClassIsland 使用的那套字体）。它自带 hinting 指令，字形会吸附到像素网格上，所以小字号下笔画依然锐利。

在没有该字体的系统上（或非 Windows 平台），会回退到随程序分发的 **HarmonyOS Sans SC Medium**（`resources/fonts/`，未修改的原始文件）。字体许可证见 `resources/fonts/LICENSE.txt`，第三方声明见 `THIRD_PARTY_NOTICES.md`。

界面整体按 **1.3×** 缩放（正文约 22px、分组标题约 29px，对应 17px 的雅黑 UI 字号），窗口默认尺寸也从 591×381 放大到约 768×495。

程序还会自动乘以系统的显示缩放比例（Windows 设置里的 100% / 125% / 150%），因此在高 DPI 屏幕上界面不会比资源管理器等系统程序小一圈。如果你觉得还大/还小，可以设置环境变量后重启程序，无需重新构建（该值是基础倍数，系统显示缩放仍会叠加在其上）：

```powershell
$env:BLUR_UI_SCALE = "1.45"   # 基础倍数，允许范围 0.5 ~ 4，默认 1.3
.\blur-Windows-Release-x64.exe
```

该缩放会同时作用于字号、控件尺寸、内边距和窗口大小，因此不会出现文字变大但控件还是小的错位。

### 构建说明

- Windows 构建需要 MSVC，本项目已在 `CMakeLists.txt` 中启用 `/utf-8`，因此中文源码无需 BOM 即可正确编译。
- 字体文件会在构建后自动复制到可执行文件旁的 `fonts` 目录；安装包也会包含该目录。
- 若字体文件缺失，程序会回退到内置的拉丁字体并记录一条错误日志（中文将显示为占位符）。

> 注意：汉化改动不会自动跟随上游更新。上游发布新版本后，需要重新合并本分支的改动并重新构建。


## Download

- [Windows installer](https://github.com/f0e/blur/releases/latest/download/blur-Windows-Installer-x64.exe)
- [macOS installer](https://github.com/f0e/blur/releases/latest/download/blur-macOS-Release-arm64.dmg)
- [Linux (requires manual installation of dependencies)](https://github.com/f0e/blur/releases/latest/download/blur-Linux-Release-x64.tar.gz)

### Beta releases

I often release beta versions with new functionality before I think they're stable enough for a proper release. To test these releases, [visit the Releases tab of the repo.](https://github.com/f0e/blur/releases) To receive beta update notifications you can enable `include beta updates` in your `blur.cfg` found in your config folder. Any feedback or issue reporting on these releases is greatly appreciated :)

### macOS notes

> After opening on Mac for the first time you'll get a 'Blur is damaged and can't be opened.' error. To fix this, run `xattr -dr com.apple.quarantine /Applications/blur.app` in Terminal to unquarantine it.\*

> The default interpolation program on macOS is RIFE, unlike Windows and Linux. RIFE is more accurate than SVP, but quite a bit slower. The reason for this difference is because because using SVP for interpolation on macOS requires that [SVP Manager](https://www.svp-team.com/get/) be running, or you'll get a red border around videos. (This software is paid, and not affiliated with Blur)

### Linux notes

Requires manual installation of dependencies. [See here for the list of dependencies.](#linux-dependency-requirements)

## Features

The amount of motion blur is easily configurable, and there are additional options to enable other features such as interpolating the video's fps. This can be used to generate 'fake' motion blur through frame blending the interpolated footage. This motion blur does not blur non-moving parts of the video, like the HUD in gameplay footage.

The program can also be used in the command line via `blur-cli`, use -h or --help for more information.

## Sample output

### 600fps footage, blurred with 0.6 blur amount

![600fps footage, blurred with 0.6 blur amount](https://i.imgur.com/Hk0XIPe.jpg)

### 60fps footage, interpolated to 600fps, blurred with 0.6 blur amount

![60fps footage, interpolated to 600fps, blurred with 0.6 blur amount](https://i.imgur.com/I4QFWGc.jpg)

As visible from these images, the interpolated 60fps footage produces motion blur that is comparable to actual 600fps footage.

---

## Recommended settings for gameplay footage:

Most of the default settings are what I find work the best, but some settings can depend on your preferences.

### Blur amount

For 60fps footage:

| intent                  | amount  |
| ----------------------- | ------- |
| Maximum blur/smoothness | >1      |
| Normal blur             | 1       |
| Medium blur             | 0.5     |
| Low blur                | 0.2-0.3 |

To preserve your old blur amount when changing framerate use the following formula:

`[new blur amount] = [old blur amount] × ([new fps] / [old fps])`

So normal blur at 30fps becomes 0.5, etc.

### Interpolated fps

Results can become worse if this is too high. In general I recommend around 5x the input fps. Also SVP seems to only be able to interpolate up to 10x the input fps, so don't bother trying anything higher than that.

## Notes

### Limiting smearing

Using blur on 60fps footage results in clean motion blur, but occasionally leaves some smearing artifacts. To remove these artifacts, higher framerate source footage can be used. Recording with software such as OBS at framerates like 120/180fps will result in a greatly reduced amount of artifacting.

### Preventing unsmooth output

If your footage contains duplicate frames then occasionally blurred frames will look out of place, making the video seem unsmooth at points. The 'deduplicate' option will automatically fill in duplicated frames with interpolated frames to prevent this from happening.

### Frameserver output

Blur supports rendering from frameservers. This means you can avoid having to run blur on your input videos when video editing. When rendering, simply output (make sure your project is high framerate) to the frameserver and then drag the generated AVI into blur. Note that some video editing software might limit the maximum project framerate.

## Config settings explained:

### blur

- blur - whether or not the output video file will have motion blur
- blur amount - if blur is enabled, this is the amount of motion blur (0 = no blur, 1 = fully blend every frame together, 1+ = more blur/ghosting)
- blur output fps - if blur is enabled, this is the fps the output video will be. can be a framerate (e.g. 600) or a multiplier (e.g. 5x)
- blur weighting - weighting function to use when blending frames. options are listed below. also: [view weighting comparison graphs.](tests/plot_weighting_functions/weighting_functions.pdf)
  - equal - each frame is blended equally
  - gaussian_sym
  - vegas
  - pyramid
  - gaussian
  - ascending
  - descending
  - gaussian_reverse
  - custom weights - custom comma-separated frame weights, e.g. 5, 3, 3, 2, 1. higher numbers indicate frames being more visible when blending, lower numbers mean they are less so.

### interpolation

- interpolate - whether or not the input video file will be interpolated to a higher fps
- interpolated fps - if interpolate is enabled, this is the fps that the input file will be interpolated to (before blurring). can be a set fps number or a multiplier (append x to end e.g. `5x`)
- interpolation method - method used for interpolation:
  - Quality: RIFE > svp
  - Speed: svp > RIFE
  - Note: On macOS, SVP requires SVP Manager to be open or a red border will appear. It provides a 30-day trial, but then costs $24.99 for a lifetime license. RIFE can always be used however, but it is slower than SVP.

### pre-interpolation

- pre-interpolation - enable pre-interpolation using a more accurate but slower AI model before main interpolation
- pre-interpolated fps - FPS to pre-interpolate input video to (before blurring). can be a set fps number or a multiplier (append x to end e.g. `5x`)

### rendering

- quality - [crf](https://trac.ffmpeg.org/wiki/Encode/H.264#crf) of the output video (may be different if using GPU encoding) - (0 = lossless quality, 51 = really bad)
- deduplicate - removes duplicate frames and generates new interpolated frames to take their place. fixes 'unsmooth' looking output caused by stuttering in recordings
- deduplicate range - amount of frames beyond the current frame to look for unique frames when deduplicating. make it higher if your footage is at a lower FPS than it should be (e.g. choppy 120fps gameplay recorded at 240fps), lower it if your blurred footage starts blurring static elements such as menu screens
- deduplicate threshold - threshold of movement that triggers deduplication. turn on debug in advanced and render a video to embed text showing the movement in each frame
- deduplicate method - method used for deduplication:
  - Quality: RIFE > svp
  - Speed: old > svp > RIFE
- preview - opens a render preview window
- detailed filenames - adds blur settings to generated filenames
- copy dates - copies over the modified date from the input file to the output file

### gpu acceleration

- gpu decoding - uses gpu when decoding
- gpu interpolation - uses gpu when interpolating
- gpu encoding - uses gpu when rendering
- gpu type (nvidia/amd/intel) - your gpu type

### timescale

- input timescale - timescale of the input video file (will be sped up/slowed down accordingly)
- output timescale - timescale of the output video file
- adjust timescaled audio pitch - will pitch shift audio when sped up/slowed down

### filters

- brightness - brightness of the output video
- saturation - saturation of the output video
- contrast - contrast of the output video

### advanced rendering

- video container - the output video container format (e.g. `mp4`, `mkv`, `avi`)
- custom ffmpeg filters - custom ffmpeg filters to be used when rendering (replaces gpu & quality options)
- debug - shows debug window, prints commands used by blur

### advanced blur

- blur weighting gaussian std dev - standard deviation used in the gaussian weighting
- blur weighting gaussian mean - mean used in the gaussian weighting
- blur weighting gaussian bound - bound used in the gaussian weighting

### advanced interpolation

- SVP interpolation preset - preset used for framerate interpolation when using SVP, one of:
  - weak (default) - _[explained further here](https://www.spirton.com/uploads/InterFrame/InterFrame2.html)_
  - film - _[explained further here](https://www.spirton.com/uploads/InterFrame/InterFrame2.html)_
  - smooth - _[explained further here](https://www.spirton.com/uploads/InterFrame/InterFrame2.html)_
  - default _(default svp settings)_
- SVP interpolation algorithm - algorithm used for framerate interpolation when using SVP, one of:

  - 13 - best overall quality and smoothness (default) - _[explained further here](https://www.spirton.com/uploads/InterFrame/InterFrame2.html)_
  - 23 - sometimes smoother than 13, but can result in smearing - _[explained further here](https://www.spirton.com/uploads/InterFrame/InterFrame2.html)_
  - 1 - _[explained further here](https://www.svp-team.com/wiki/Manual:SVPflow)_
  - 2 - _[explained further here](https://www.spirton.com/uploads/InterFrame/InterFrame2.html)_
  - 11 - _[explained further here](https://www.svp-team.com/wiki/Manual:SVPflow)_
  - 21 - _[explained further here](https://www.svp-team.com/wiki/Manual:SVPflow)_

- interpolation block size - block size used for framerate interpolation. higher block size = less accurate blur, will result in spaces around non-moving objects of the frame, also renders faster. lower block size = more accurate blur, but can result in artifacting, also slower. for higher framerate input videos lower block size can be better. options:

  - 4
  - 8 (default)
  - 16
  - 32

- interpolation mask area - mask amount used when interpolating. higher values can mean static objects are blurred less, but can also result in less smooth output (moving parts of the image can be mistaken for static parts and don't get blurred)

### manual svp override

You can customise the SVP interpolation settings even further by manually defining json parameters. [see here](https://www.svp-team.com/wiki/Manual:SVPflow) for explanations on settings

- manual svp: enables manual svp settings, true/false
- super string: json string used as input in [SVSuper](https://www.svp-team.com/wiki/Manual:SVPflow#SVSuper.28source.2C_params_string.29)
- vectors string: json string used as input in [SVAnalyse](https://www.svp-team.com/wiki/Manual:SVPflow#SVAnalyse.28super.2C_params_string.2C_.5Bsrc.5D:_clip.29)
- smooth string: json string used as input in [SVSmoothFps](https://www.svp-team.com/wiki/Manual:SVPflow#SVSmoothFps.28source.2C_super.2C_vectors.2C_params_string.2C_.5Bsar.5D:_float.2C_.5Bmt.5D:_integer.29)

These options are not visible by default, add them to your config and they will be used.

## Linux dependency requirements

### General list of things you need

If your distro isn't listed below, here's a list of the things you'll need to install.

- VapourSynth
- FFmpeg
- VapourSynth plugins (install to your system vapoursynth plugin path or [your blur binary directory]/vapoursynth-plugins)
  - [SVPflow](https://web.archive.org/web/20190322064557/http://www.svp-team.com/files/gpl/svpflow-4.2.0.142.zip)
  - [BestSource](https://github.com/vapoursynth/bestsource) ([my automated build](https://github.com/f0e/blur-plugin-builds/releases/latest))
  - [MVTools](https://github.com/dubhater/vapoursynth-mvtools) ([my automated build](https://github.com/f0e/blur-plugin-builds/releases/latest))
  - [Akarin](https://github.com/AkarinVS/vapoursynth-plugin) (or this [fork which supports newer LLVM versions](https://github.com/Jaded-Encoding-Thaumaturgy/akarin-vapoursynth-plugin)) ([my automated build](https://github.com/f0e/blur-plugin-builds/releases/latest))
  - [RIFE-ncnn-Vulkan](https://github.com/styler00dollar/VapourSynth-RIFE-ncnn-Vulkan/releases/latest)
  - [Adjust](https://github.com/f0e/Vapoursynth-adjust/releases/latest)

### Arch required packages

`paru -S vapoursynth ffmpeg vapoursynth-plugin-svpflow vapoursynth-plugin-bestsource vapoursynth-plugin-mvtools vapoursynth-plugin-vsakarin-av1an-git vapoursynth-plugin-rife-ncnn-vulkan`

And manually install [adjust](https://github.com/f0e/Vapoursynth-adjust/releases/latest)

---

\*in the future I might buy a dev cert, but $99 a year atm doesn't seem worth it 😅
