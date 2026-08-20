param(
    [Parameter(Mandatory=$true)][string]$Project,
    [switch]$SkipDependencyFetch
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$Root = [System.IO.Path]::GetFullPath($Root)
$LogDir = Join-Path $Root "build_logs"
New-Item -ItemType Directory -Force -Path $LogDir | Out-Null
$LogPath = Join-Path $LogDir ($Project + ".log")

$transcriptStarted = $false
try {
    try {
        Start-Transcript -Path $LogPath -Force | Out-Null
        $transcriptStarted = $true
    }
    catch {
        Write-Host "WARNING: Could not start transcript: $($_.Exception.Message)"
    }

    Write-Host "============================================================"
    Write-Host "GInputNext build: $Project"
    Write-Host "Root: $Root"
    Write-Host "Log:  $LogPath"
    Write-Host "============================================================"

    if (-not $env:PLUGIN_SDK_DIR) {
        throw "PLUGIN_SDK_DIR is not set."
    }

    if (-not $SkipDependencyFetch) {
        Write-Host "Preparing SDL2 x86 dependency..."
        & (Join-Path $Root "tools\FETCH_SDL2.ps1") -Root $Root

        Write-Host "Preparing controller mapping database..."
        & (Join-Path $Root "tools\FETCH_CONTROLLER_DB.ps1") -Root $Root
    }
    else {
        Write-Host "Skipping dependency fetch (already prepared by BUILD_ALL)."
    }

    $SDL2 = Join-Path $Root "third_party\SDL2-2.32.10"
    $sdlHeader = Join-Path $SDL2 "include\SDL.h"
    $sdlDll = Join-Path $SDL2 "lib\x86\SDL2.dll"
    $dbFile = Join-Path $Root "third_party\SDL_GameControllerDB\gamecontrollerdb.txt"

    foreach ($required in @($sdlHeader, $sdlDll, $dbFile)) {
        if (-not (Test-Path $required)) {
            throw "Required dependency is missing: $required"
        }
    }

    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vswhere)) {
        throw "vswhere.exe not found. Install Visual Studio 2022 / Build Tools."
    }

    $msbuild = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe | Select-Object -First 1
    if (-not $msbuild) {
        throw "MSBuild not found."
    }

    $dumpbin = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find VC\Tools\MSVC\**\bin\Hostx64\x86\dumpbin.exe | Select-Object -First 1
    if (-not $dumpbin) {
        $dumpbin = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find VC\Tools\MSVC\**\bin\Hostx86\x86\dumpbin.exe | Select-Object -First 1
    }
    if (-not $dumpbin) {
        throw "x86 dumpbin.exe not found. Install the VS2022 x86/x64 C++ tools."
    }

    Write-Host "MSBuild: $msbuild"
    Write-Host "dumpbin: $dumpbin"

    $headers = & $dumpbin /headers $sdlDll 2>&1 | Out-String
    if ($LASTEXITCODE -ne 0) {
        throw "dumpbin /headers failed for SDL2.dll with exit code $LASTEXITCODE"
    }
    if ($headers -notmatch '(?im)\b14C machine \(x86\)') {
        throw "The downloaded SDL2.dll is not an x86/32-bit PE DLL."
    }

    $exports = & $dumpbin /exports $sdlDll 2>&1 | Out-String
    if ($LASTEXITCODE -ne 0) {
        throw "dumpbin /exports failed for SDL2.dll with exit code $LASTEXITCODE"
    }

    $requiredExports = @(
        "SDL_GetVersion",
        "SDL_InitSubSystem",
        "SDL_NumJoysticks",
        "SDL_GameControllerOpen",
        "SDL_GameControllerGetAxis",
        "SDL_GameControllerGetButton",
        "SDL_GameControllerGetType",
        "SDL_JoystickOpen",
        "SDL_JoystickIsHaptic"
    )
    foreach ($symbol in $requiredExports) {
        if ($exports -notmatch [regex]::Escape($symbol)) {
            throw "Official SDL2 x86 DLL is missing required export: $symbol"
        }
    }
    Write-Host "SDL2 runtime validation passed: 32-bit x86 + required exports."

    $projPath = Join-Path $Root ("projects\" + $Project + ".vcxproj")
    if (-not (Test-Path $projPath)) {
        throw "Project not found: $projPath"
    }

    Write-Host ""
    Write-Host "Building $Project Release|Win32..."

    $args = @(
        $projPath,
        "/m",
        "/t:Build",
        "/p:Configuration=Release",
        "/p:Platform=Win32",
        "/p:PLUGIN_SDK_DIR=$env:PLUGIN_SDK_DIR",
        "/p:SDL2_DIR=$SDL2"
    )

    # Start-Process gives us a reliable process ExitCode instead of relying on
    # a stale $LASTEXITCODE value after mixed PowerShell/native calls.
    $proc = Start-Process -FilePath $msbuild -ArgumentList $args -Wait -PassThru -NoNewWindow
    if ($proc.ExitCode -ne 0) {
        throw "MSBuild failed for $Project with exit code $($proc.ExitCode). See $LogPath"
    }

    $gameOut = if ($Project -eq "GInputNext_GTA3") { "GTA3" } elseif ($Project -eq "GInputNext_GTAVC") { "GTAVC" } else { "GTASA" }
    $outDir = Join-Path $Root ("dist\" + $gameOut + "\GInputNext")
    $outAsi = Join-Path $outDir "GInputNext.asi"
    $outDll = Join-Path $outDir "GInputNext.SDL2.dll"
    $outIni = Join-Path $outDir "GInputNext.ini"
    $outDb = Join-Path $outDir "GInputNext.gamecontrollerdb.txt"

    foreach ($f in @($outAsi, $outDll, $outIni, $outDb)) {
        if (-not (Test-Path $f)) {
            throw "Build succeeded but required packaged output is missing: $f"
        }
    }

    $outHeaders = & $dumpbin /headers $outDll 2>&1 | Out-String
    if ($LASTEXITCODE -ne 0) {
        throw "dumpbin failed on packaged SDL DLL with exit code $LASTEXITCODE"
    }
    if ($outHeaders -notmatch '(?im)\b14C machine \(x86\)') {
        throw "Packaged GInputNext.SDL2.dll is not the required 32-bit x86 DLL."
    }

    Write-Host ""
    Write-Host "SUCCESS: $Project"
    Write-Host "  $outAsi"
    Write-Host "  $outDll  [x86/32-bit]"
    Write-Host "  $outIni"
    Write-Host "  $outDb  [controller mappings]"
    exit 0
}
catch {
    Write-Host ""
    Write-Host "============================================================" -ForegroundColor Red
    Write-Host "BUILD FAILED: $Project" -ForegroundColor Red
    Write-Host $_.Exception.Message -ForegroundColor Red
    Write-Host "Build log: $LogPath" -ForegroundColor Yellow
    Write-Host "============================================================" -ForegroundColor Red
    exit 1
}
finally {
    if ($transcriptStarted) {
        try { Stop-Transcript | Out-Null } catch {}
    }
}
