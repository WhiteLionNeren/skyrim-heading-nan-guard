# x64-windows-static (static CRT, static libs) built with clang-cl + xwin on Linux.
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../../extern/CommonLibSSE/cmake/toolchain-linux-clangcl.cmake")
set(VCPKG_BUILD_TYPE release)
# The chainloaded toolchain doesn't map VCPKG_CRT_LINKAGE to a runtime library, so set /MT explicitly.
set(VCPKG_CMAKE_CONFIGURE_OPTIONS -DCMAKE_POLICY_DEFAULT_CMP0091=NEW -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded)
