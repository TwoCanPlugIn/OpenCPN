# Set ARCH in parent using cmake probes and various heuristics.
# Based on code from nohal

function(GetArch)
  # Set up ARCH
  if (NOT OCPN_TARGET_TUPLE STREQUAL "")
    list(GET OCPN_TARGET_TUPLE 2 ARCH)
  elseif (WIN32)
    # Determine the actual build target (x86 / x64 / arm64) rather than
    # always assuming x86_64 (previously done unconditionally here, see
    # #2027 for the historical reason). CMAKE_SIZEOF_VOID_P alone can't
    # tell x64 and arm64 apart (both are 8), so prefer
    # CMAKE_GENERATOR_PLATFORM (set by eg. -A x64 / -A Win32 / -A ARM64
    # with the Visual Studio generator) when available, and fall back to
    # CMAKE_SYSTEM_PROCESSOR / CMAKE_SIZEOF_VOID_P for generators (eg.
    # Ninja) that don't set CMAKE_GENERATOR_PLATFORM.
    if (CMAKE_GENERATOR_PLATFORM MATCHES "ARM64")
      set (ARCH "arm64")
    elseif (CMAKE_GENERATOR_PLATFORM STREQUAL "x64")
      set (ARCH "x86_64")
    elseif (CMAKE_GENERATOR_PLATFORM STREQUAL "Win32")
      set (ARCH "x86")
    elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|aarch64")
      set (ARCH "arm64")
    elseif (CMAKE_SIZEOF_VOID_P EQUAL 8)
      set (ARCH "x86_64")
    else ()
      set (ARCH "x86")
    endif ()
  else ()
    # Defaults:
    set (ARCH "x86_64")
    if (CMAKE_SYSTEM_PROCESSOR MATCHES "arm*")
      if (CMAKE_SIZEOF_VOID_P MATCHES "8")
        set (ARCH "arm64")
      else ()
        set (ARCH "armhf")
      endif ()
    else (CMAKE_SYSTEM_PROCESSOR MATCHES "arm*")
      set (ARCH ${CMAKE_SYSTEM_PROCESSOR})
    endif ()
    if (ARCH STREQUAL "arm64")
      if (OCPN_FLATPAK)
        set(ARCH "aarch64")
      elseif (EXISTS /etc/redhat-release)
        set (ARCH "aarch64")
      elseif (EXISTS /etc/suse-release OR EXISTS /etc/SuSE-release)
        set (ARCH "aarch64")
      endif ()
    endif ()
  endif ()

  # On Windows, also expose plain boolean x86 / x64 / arm64 variables (in
  # addition to ARCH) so that other CMakeLists.txt files can branch simply
  # on:
  #
  #   if (x86)
  #     ...
  #   elseif (x64)
  #     ...
  #   elseif (arm64)
  #     ...
  #   endif ()
  #
  # instead of each repeating its own ARCH string comparisons. Since
  # GetArch() is called once from the top-level CMakeLists.txt before any
  # add_subdirectory() calls, these PARENT_SCOPE variables land in the
  # top-level directory scope and are automatically inherited by every
  # subdirectory added afterwards - no further plumbing needed in each
  # plugin/library's own CMakeLists.txt.
  # NOTE caching of prebuilt bundled Windows libraries
  # x86 remain unchanged in cache/buildwin
  # x64 placed in cache/buildwin64
  # NOTE for ARM64 support: many of the prebuilt/bundled Windows binaries
  # referenced elsewhere in the build (eg. cache/buildwin/*.lib static
  # blobs, CrashRpt) only exist as x86 or x64 builds today. Adding an
  # `elseif (arm64)` branch that reuses the x64 path's libraries will NOT
  # work - those need genuine ARM64 builds (or an ARM64 vcpkg triplet, eg.
  # arm64-windows) before an arm64 branch can link successfully.
  if (WIN32)
    set (x86 FALSE)
    set (x64 FALSE)
    set (arm64 FALSE)
    if (ARCH STREQUAL "x86_64" OR ARCH STREQUAL "x64" OR ARCH STREQUAL "AMD64")
      set (x64 TRUE)
    elseif (ARCH STREQUAL "x86" OR ARCH STREQUAL "i386" OR ARCH STREQUAL "i686" OR ARCH STREQUAL "Win32")
      set (x86 TRUE)
    elseif (ARCH STREQUAL "arm64" OR ARCH STREQUAL "ARM64" OR ARCH STREQUAL "aarch64")
      set (arm64 TRUE)
    else ()
      message(WARNING "GetArch: Unrecognized Windows ARCH '${ARCH}' - x86/x64/arm64 not set")
    endif ()
    set (x64 ${x64} PARENT_SCOPE)
    set (x86 ${x86} PARENT_SCOPE)
    set (arm64 ${arm64} PARENT_SCOPE)
  endif ()

  set(ARCH ${ARCH} PARENT_SCOPE)
endfunction (GetArch)
