# Heading NaN Guard

An SKSE plugin for Skyrim Special Edition / Anniversary Edition that stops a class of **permanent freezes**: the game hangs
(no crash, no crash log, never recovers), usually in the open world a few seconds after NPCs or animals load in.

## The bug it fixes

The engine converts a 2D direction into a heading angle with a small helper (Address Library ID **70172**). It computes
`atan(x / y)`, adds π when `y < 0`, then wraps the result into [0, 2π] with a loop:

```
while (angle < 0) angle += 2π;
```

If the direction contains **NaN**, every comparison is false-and-unordered, so the loop never ends. The helper runs on the
AI/movement job threads (actor movement → path follower update, ID 90711), and the main thread waits for that job every
frame (`WaitForJobTask`, ID 69516). The result is a frozen game.

Vanilla rarely produces a NaN direction, but **mods that change NPC movement or pathing can**. It was found with a full
memory dump of a freeze in a heavily modded game, where a pathing mod produced a NaN direction for an animal every few
minutes.

## What it does

It replaces the helper with an identical implementation (same constants and wrapping) that **returns 0 for a NaN or infinite
input** instead of looping. An affected NPC faces north for a frame instead of freezing the game.

Each bad input is logged with the address of the caller, so you can see which code produced the NaN:

```
Documents/My Games/Skyrim Special Edition/SKSE/HeadingNaNGuard.log
[20:02:10.429] [warning] bad vector (-nan, 4.0177e-41) from SkyrimSE.exe+11B0D00 (hit 1)
```

It logs the first 50 hits and then every 1000th. No hits means nothing in your game produced a NaN direction.

## Requirements and compatibility

- SKSE64 and [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444).
- **Game 1.6.1170** is tested. Before patching, the plugin checks the function's first bytes. On any other version where
  they don't match, it logs `doesn't match the expected code: NOT patched` and does nothing, so it is safe to install
  anywhere.
- No ESP, no scripts, nothing stored in your save. Safe to add or remove at any time.

## Install

Copy `SKSE/Plugins/HeadingNaNGuard.dll` into your `Data` folder, or install the release zip with a mod manager.

## Building

The source is `src/main.cpp`; it uses [CommonLibSSE-NG](https://github.com/alandtse/CommonLibVR) v8.3.0 as a git submodule.

```bash
git clone --recursive https://github.com/WhiteLionNeren/skyrim-heading-nan-guard
```

**Windows (MSVC, not tested by us; releases are built on Linux):** configure with vcpkg's toolchain file and the static triplet, then build:

```
cmake -B build -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static
cmake --build build --config Release
```

**Linux (cross-compile with clang-cl):** `linux/build.sh`. The script's header lists what it needs: LLVM 20, an
[xwin](https://github.com/Jake-Shadle/xwin) MSVC sysroot, vcpkg, and Wine + llvm-mingw for DirectXTK's shader step.

## License

**This repository's own source is public domain** ([Unlicense](LICENSE)).

**The compiled DLL is GPL-3.0-or-later.** It statically links
[CommonLibSSE-NG](https://github.com/alandtse/CommonLibVR) v8.3.0, which is GPL-3.0-or-later with a Modding Exception and a
GPL-3.0 Linking Exception (see `extern/CommonLibSSE/COPYING.txt` and `EXCEPTIONS.md`), so the binary is a combined work under
those terms. This repository, with its pinned submodule, is the binary's Corresponding Source. The release zip carries a
`NOTICE.txt` and the GPL text.

By WhiteLionNeren and Claude (Anthropic).
