param([string]$Root)
$ErrorActionPreference = 'Stop'
if (-not $Root) { $Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path) }

Write-Host "[Buildbot] Building..."
& "$Root\build_all.bat"

Write-Host "[Buildbot] Running tests..."
python "$Root\tools\run_tests.py"

Write-Host "[Buildbot] Running benchmarks..."
python "$Root\tools\run_benchmarks.py"

Write-Host "[Buildbot] Done."
