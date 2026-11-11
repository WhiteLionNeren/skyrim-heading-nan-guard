#!/bin/bash
# Cross-compile HeadingNaNGuard.dll on Linux: clang-cl + lld-link (LLVM 20), an MSVC sysroot from xwin, vcpkg.
# Needs: git submodule update --init --recursive; clang-cl/lld-link/llvm-* on PATH (or *-20 names); ninja; cmake >= 3.21;
#        xwin sysroot (xwin --accept-license splat --include-debug-libs --use-winsysroot-style --preserve-ms-arch-notation
#        --output ~/xwin-out); vcpkg checkout; wine64 + llvm-mingw (DirectXTK compiles its shaders with fxc2 under Wine).
# Env: VCPKG_ROOT (default ~/vcpkg), XWIN_SYSROOT (default ~/xwin-out), LLVM_MINGW_BIN, WINEPREFIX,
#      VCPKG_INSTALLED (reuse an existing vcpkg_installed tree instead of building the packages).
set -euo pipefail
cd "$(dirname "$0")/.."
SRC=$PWD
CL=${COMMONLIB:-$SRC/extern/CommonLibSSE}
export VCPKG_ROOT=${VCPKG_ROOT:-$HOME/vcpkg}
export XWIN_SYSROOT=${XWIN_SYSROOT:-$HOME/xwin-out}
export LLVM_MINGW_BIN=${LLVM_MINGW_BIN:-$HOME/llvm-mingw/bin}
export WINEPREFIX=${WINEPREFIX:-$HOME/.cache/skse-build-wine}
export WINEDEBUG=-all
export VCPKG_KEEP_ENV_VARS="XWIN_SYSROOT;LLVM_MINGW_BIN;WINEPREFIX;WINEDEBUG"
WARN="-Wno-overloaded-virtual -Wno-delete-non-abstract-non-virtual-dtor -Wno-inconsistent-missing-override -Wno-reinterpret-base-class \
-Wno-nonportable-include-path -Wno-deprecated-copy-with-user-provided-copy -Wno-invalid-offsetof -Wno-switch -Wno-unused-command-line-argument \
-Wno-deprecated-volatile -Wno-unused-private-field -Wno-microsoft-include -Wno-unused-parameter -Wno-four-char-constants -Wno-multichar"
EXTRA=()
# -ffile-prefix-map: source paths embedded in the DLL (log source locations) become repo-relative.
MAP="/clang:-ffile-prefix-map=$SRC/="
[ -n "${VCPKG_INSTALLED:-}" ] && MAP="$MAP /clang:-ffile-prefix-map=$VCPKG_INSTALLED/=vcpkg_installed/"
[ -n "${VCPKG_INSTALLED:-}" ] && EXTRA+=(-DVCPKG_INSTALLED_DIR="$VCPKG_INSTALLED" -DVCPKG_MANIFEST_INSTALL=OFF)
cmake -S "$SRC" -B "$SRC/build-linux" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCommonLibPath="$CL" \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
  -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE="$CL/cmake/toolchain-linux-clangcl.cmake" \
  -DVCPKG_OVERLAY_TRIPLETS="$SRC/linux/triplets" \
  -DVCPKG_OVERLAY_PORTS="$CL/examples/linux-cross-compile/custom-ports" \
  -DVCPKG_TARGET_TRIPLET=x64-windows-static-clangcl \
  -DBUILD_TESTS=OFF "${EXTRA[@]}" \
  -DCMAKE_CXX_FLAGS="/EHsc /DNOMINMAX /W4 /external:anglebrackets /external:W0 /Zc:inline /Zc:preprocessor $MAP $WARN"
cmake --build "$SRC/build-linux" --target HeadingNaNGuard
ls -l "$SRC/build-linux/HeadingNaNGuard.dll"
