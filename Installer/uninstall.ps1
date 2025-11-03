param(
  [string]$Prefix = "$env:USERPROFILE\Rubolt"
)

$ErrorActionPreference = 'Stop'

if(-not (Test-Path $Prefix)){
  Write-Warning "Prefix does not exist: $Prefix"
}

$bin = Join-Path $Prefix 'bin'
$lib = Join-Path $Prefix 'lib\rubolt'
$share = Join-Path $Prefix 'share\rubolt'

foreach($p in @($bin,$lib,$share)){
  if(Test-Path $p){
    Write-Host "Removing $p"
    Remove-Item -Recurse -Force $p
  }
}

# Best-effort PATH cleanup (cannot reliably edit user PATH entries removal without full parse)
Write-Host "Uninstall complete. You may remove $bin from your PATH if present."
