param(
    [string]$Root = (Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)),
    [switch]$Force
)

$ErrorActionPreference = "Stop"

# Normalize caller-provided paths. In cmd.exe a quoted argument ending in "\"
# can be parsed differently by downstream runtimes; trim any stray quote and
# canonicalize before using Join-Path/Test-Path.
if ($Root) {
    $Root = $Root.Trim().Trim('"')
    $Root = [System.IO.Path]::GetFullPath($Root)
}

$version = "2.32.10"
$third = Join-Path $Root "third_party"
$dest = Join-Path $third "SDL2-$version"
$zip = Join-Path $third "SDL2-devel-$version-VC.zip"
$url = "https://github.com/libsdl-org/SDL/releases/download/release-$version/SDL2-devel-$version-VC.zip"

$header = Join-Path $dest "include\SDL_version.h"
$dll = Join-Path $dest "lib\x86\SDL2.dll"

Write-Host "Canonical dependency root: $Root"
Write-Host "SDL header path: $header"
Write-Host "SDL x86 DLL path: $dll"

function Test-ExpectedSDL {
    if (-not (Test-Path $header)) { return $false }
    if (-not (Test-Path $dll)) { return $false }

    $text = Get-Content -Raw $header
    if ($text -notmatch '#define\s+SDL_MAJOR_VERSION\s+2') { return $false }
    if ($text -notmatch '#define\s+SDL_MINOR_VERSION\s+32') { return $false }
    if ($text -notmatch '#define\s+SDL_PATCHLEVEL\s+10') { return $false }

    # Verify PE machine == IMAGE_FILE_MACHINE_I386 (0x014C).
    $fs = [System.IO.File]::OpenRead($dll)
    try {
        $br = New-Object System.IO.BinaryReader($fs)
        $fs.Position = 0x3C
        $peOffset = $br.ReadInt32()
        $fs.Position = $peOffset
        $sig = $br.ReadUInt32()
        if ($sig -ne 0x00004550) { return $false }
        $machine = $br.ReadUInt16()
        if ($machine -ne 0x014C) {
            Write-Host ("Wrong SDL2 DLL machine: 0x{0:X4}; expected x86 0x014C" -f $machine)
            return $false
        }
    }
    finally {
        $fs.Dispose()
    }

    return $true
}

if (-not $Force -and (Test-ExpectedSDL)) {
    Write-Host "SDL2 $version x86 already present and verified."
    return
}

New-Item -ItemType Directory -Force -Path $third | Out-Null

if (Test-Path $dest) {
    Remove-Item -Recurse -Force $dest
}
if ($Force -and (Test-Path $zip)) {
    Remove-Item -Force $zip
}

Write-Host "Downloading official SDL2 $version Visual C++ development package..."
Write-Host $url
Invoke-WebRequest -UseBasicParsing -Uri $url -OutFile $zip

$temp = Join-Path $third "_sdl_extract"
if (Test-Path $temp) { Remove-Item -Recurse -Force $temp }
Expand-Archive -Path $zip -DestinationPath $temp -Force

$found = Get-ChildItem -Path $temp -Directory |
    Where-Object { $_.Name -eq "SDL2-$version" } |
    Select-Object -First 1

if (-not $found) {
    throw "Could not find SDL2-$version inside the official archive."
}

Move-Item $found.FullName $dest
Remove-Item -Recurse -Force $temp

if (-not (Test-ExpectedSDL)) {
    throw "Downloaded SDL2 package failed version/x86 verification."
}

Write-Host "SDL2 $version x86 verified:"
Write-Host "  $dll"
