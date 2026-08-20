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

$commit = "42f28e22d20761e7004e8db91c4ad86402fdf600"
$third = Join-Path $Root "third_party\SDL_GameControllerDB"
$outFile = Join-Path $third "gamecontrollerdb.txt"
$url = "https://raw.githubusercontent.com/mdqinc/SDL_GameControllerDB/$commit/gamecontrollerdb.txt"

Write-Host "Canonical dependency root: $Root"
Write-Host "Controller DB path: $outFile"

function Test-ControllerDB {
    if (-not (Test-Path $outFile)) { return $false }
    $text = Get-Content -Raw $outFile
    if ($text -notmatch '# Game Controller DB for SDL') { return $false }
    if ($text -notmatch 'platform:Windows') { return $false }
    return $true
}

New-Item -ItemType Directory -Force -Path $third | Out-Null
if ($Force -or -not (Test-ControllerDB)) {
    Write-Host "Downloading pinned SDL_GameControllerDB..."
    Invoke-WebRequest -UseBasicParsing -Uri $url -OutFile $outFile
}

if (-not (Test-ControllerDB)) {
    throw "Controller DB download/validation failed."
}

Write-Host "Controller DB ready at $outFile (commit $commit)"
