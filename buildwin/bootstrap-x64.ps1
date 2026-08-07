# Powershell script for 64 bit OpenCPN builds
# Downloads and build the dependencies.
# Note vcpkg will detect if the packages have already been downloaded and built

$localVcpkgDir = Join-Path $PSScriptRoot "vcpkg"

# Some vcpkg distributions - notably the one bundled with Visual Studio
# since 17.6 - are built WITHOUT classic mode support at all ("This vcpkg
# distribution does not have a classic mode instance"). That's not fixable
# by adding a vcpkg.json; that particular binary just can't do
# `vcpkg install <port>`. So rather than trust that any vcpkg.exe we find
# can do what this script needs, actually probe it with a harmless
# --dry-run before accepting it.
function Test-VcpkgClassicMode {
  param([string]$exe)
  try {
    $output = & $exe install zlib --triplet x64-windows --dry-run 2>&1 | Out-String
  } catch {
    return $false
  }
  if ($output -match "does not have a classic mode instance" -or
      $output -match "Could not locate a manifest") {
    return $false
  }
  return $true
}

# Locate a vcpkg.exe that actually supports classic mode, in order of
# preference:
#   1. A local .\vcpkg checkout already managed by this script.
#   2. $env:VCPKG_ROOT, if set.
#   3. vcpkg.exe already on PATH.
# Each candidate is probed with Test-VcpkgClassicMode before being
# accepted - eg. a manifest-only Visual-Studio-bundled copy on PATH will
# be skipped rather than blindly used and failing later. If nothing
# usable is found, fall back to cloning a fresh local copy (which is
# always classic-mode capable).
function Find-Vcpkg {
  $candidates = @()

  $localExe = Join-Path $localVcpkgDir "vcpkg.exe"
  if (Test-Path $localExe) { $candidates += $localExe }

  if ($env:VCPKG_ROOT) {
    $rootExe = Join-Path $env:VCPKG_ROOT "vcpkg.exe"
    if (Test-Path $rootExe) { $candidates += $rootExe }
  }

  $onPath = Get-Command "vcpkg.exe" -ErrorAction SilentlyContinue
  if ($onPath) { $candidates += $onPath.Source }

  foreach ($candidate in $candidates) {
    Write-Host "Checking vcpkg candidate: $candidate"
    if (Test-VcpkgClassicMode $candidate) {
      return $candidate
    }
    Write-Host "  -> not usable (no classic mode support), skipping"
  }

  return $null
}

$vcpkgExe = Find-Vcpkg

if (-not $vcpkgExe) {
  Write-Host "No existing classic-mode-capable vcpkg found - cloning a fresh copy into $localVcpkgDir"
  git clone https://github.com/microsoft/vcpkg $localVcpkgDir
  & (Join-Path $localVcpkgDir "bootstrap-vcpkg.bat") -disableMetrics
  $vcpkgExe = Join-Path $localVcpkgDir "vcpkg.exe"
}

Write-Host "Using vcpkg: $vcpkgExe"

# Download and build the dependencies
& $vcpkgExe install --triplet x64-windows openssl curl libarchive glew portaudio pthread getopt libusb portaudio libsndfile crashrpt
if ($LASTEXITCODE -ne 0) {
  throw "vcpkg install failed with exit code $LASTEXITCODE"
}

# $src below must match whichever vcpkg instance was actually used above -
# derive it from $vcpkgExe's own location rather than assuming the local
# .\vcpkg checkout, since we may have used VCPKG_ROOT or PATH instead.
$vcpkgRoot = Split-Path $vcpkgExe -Parent

# Copy dependencies & curl certificate to the cache
# This may need cleaning up if developing both x86 & x64 (and perhaps arm64) to stop clobbering each other in the cache

$src = Join-Path $vcpkgRoot "installed\x64-windows"
$dest  = "..\cache\buildwin"

mkdir $dest -Force

# curl
copy $src\lib\libcurl.lib $dest
copy $src\bin\libcurl.dll $dest

# zlib
copy $src\lib\z.lib $dest
copy $src\bin\z.dll $dest

# openssl
copy $src\lib\libssl.lib $dest
copy $src\lib\libcrypto.lib $dest
copy $src\bin\libssl-3-x64.dll $dest
copy $src\bin\libcrypto-3-x64.dll $dest
mkdir $dest\include\openssl -Force
copy $src\include\openssl\* $dest\include\openssl\

# curl headers (model/CMakeLists.txt sets CURL_INCLUDE_DIRS to cache/buildwin/include)
mkdir $dest\include\curl -Force
copy $src\include\curl\* $dest\include\curl\

# libarchive
copy $src\lib\archive.lib $dest
copy $src\bin\archive.dll $dest
copy $src\include\archive.h $dest\include\
copy $src\include\archive_entry.h $dest\include\

# libarchive's own runtime dependencies use vcpkg depend-info libarchive to see what it requires
copy $src\bin\liblzma.dll $dest
copy $src\bin\bz2.dll $dest
copy $src\bin\lz4.dll $dest
copy $src\bin\zstd.dll $dest

# GLEW
mkdir $dest\include\glew -Force
copy $src\include\GL\glew.h  $dest\include\glew\
copy $src\include\GL\wglew.h  $dest\include\glew\
copy $src\include\GL\eglew.h  $dest\include\glew\
copy $src\lib\glew32.lib  $dest
copy $src\bin\glew32.dll  $dest

# Portaudio
copy $src\bin\portaudio.dll $dest
copy $src\bin\sndfile.dll $dest
copy $src\bin\opus.dll $dest
copy $src\bin\mpg123.dll $dest
copy $src\bin\libmp3lame.dll $dest
copy $src\bin\vorbis.dll $dest
copy $src\bin\vorbisenc.dll $dest
copy $src\bin\flac.dll $dest
copy $src\bin\ogg.dll $dest

# Crashrpt
mkdir $dest\crashrpt -Force
copy $src\tools\crashrpt\crashrpt_lang.ini $dest\crashrpt
copy $src\tools\crashrpt\crashsender1403.exe $dest\crashrpt
copy $src\lib\crashrpt1403.lib $dest\crashrpt
copy $src\include\crashrpt\crashrpt.h $dest\crashrpt
copy $src\bin\crashrpt1403.dll $dest\crashrpt
copy $src\bin\dbghelp.dll $dest\crashrpt
#copy privacypolicy.txt $dest\crashrpt

# And while we're here download the curl certificate
if ( -not (Test-Path $dest\curl-ca-bundle.crt)) {
 Invoke-WebRequest https://curl.se/ca/cacert.pem -OutFile $dest\curl-ca-bundle.crt
}