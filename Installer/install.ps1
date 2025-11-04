param(
  [ValidateSet('auto','build','dist','source')][string]$Mode = 'auto',
  [string]$Prefix = "$env:USERPROFILE\Rubolt",
  [switch]$AddToPath,
  [switch]$Build
)

$ErrorActionPreference = 'Stop'

function New-Dir($p){ if(-not (Test-Path $p)){ New-Item -ItemType Directory -Path $p | Out-Null } }
function Copy-Tree($src,$dst){ New-Dir $dst; robocopy "$src" "$dst" /E /NFL /NDL /NJH /NJS /NP | Out-Null }

function Add-UserPath([string]$p){
  $cur = [Environment]::GetEnvironmentVariable('Path','User')
  if(-not $cur){ $cur = '' }
  if(-not ($cur.Split(';') -contains $p)){
    $new = ($cur.TrimEnd(';') + ';' + $p).Trim(';')
    [Environment]::SetEnvironmentVariable('Path',$new,'User')
    Write-Host "Added to PATH (User): $p"
  } else { Write-Host "PATH already contains: $p" }
}

# Resolve mode automatically
if($Mode -eq 'auto'){
  if(Test-Path 'dist'){ $Mode = 'dist' }
  elseif(Test-Path 'build'){ $Mode = 'build' }
  else { $Mode = 'source' }
}

Write-Host "[Installer] Mode=$Mode Prefix=$Prefix"

$bin = Join-Path $Prefix 'bin'
$lib = Join-Path $Prefix 'lib\rubolt'
$share = Join-Path $Prefix 'share\rubolt'

New-Dir $bin; New-Dir $lib; New-Dir $share

switch($Mode){
  'dist' {
    # Expect dist layout: dist/bin, dist/lib, dist/share
    if(Test-Path 'dist/bin'){ Copy-Tree 'dist/bin' $bin }
    if(Test-Path 'dist/lib'){ Copy-Tree 'dist/lib' $lib }
    if(Test-Path 'dist/share'){ Copy-Tree 'dist/share' $share }
  }
  'build' {
    # Copy built binaries
    if(Test-Path 'src/rubolt.exe'){ Copy-Item 'src/rubolt.exe' $bin -Force }
    if(Test-Path 'rbcli.exe'){ Copy-Item 'rbcli.exe' $bin -Force }
    elseif(Test-Path 'cli/rbcli.exe'){ Copy-Item 'cli/rbcli.exe' $bin -Force }
    # Copy runtime libraries
    foreach($d in 'StdLib','Objects','Modules'){
      if(Test-Path $d){ Copy-Tree $d (Join-Path $lib $d) }
    }
    # Share: docs, examples, python tools, vendor deps
    foreach($d in 'Docs','examples','vscode-rubolt','python','vendor'){
      if(Test-Path $d){ Copy-Tree $d (Join-Path $share $d) }
    }
  }
  'source' {
    # Copy entire project and optionally build
    Copy-Tree (Get-Location).Path (Join-Path $Prefix 'Rubolt')
    if($Build){
      Push-Location (Join-Path $Prefix 'Rubolt')
      if(Test-Path 'build_all.bat'){
        cmd /c build_all.bat
        Copy-Item 'src/rubolt.exe' (Join-Path $bin 'rubolt.exe') -Force
        if(Test-Path 'rbcli.exe'){ Copy-Item 'rbcli.exe' $bin -Force } elseif(Test-Path 'cli/rbcli.exe'){ Copy-Item 'cli/rbcli.exe' $bin -Force }
      }
      Pop-Location
    }
  }
}

# Sanity check
if(-not (Test-Path (Join-Path $bin 'rubolt.exe'))){ Write-Warning 'rubolt.exe not found in bin.' } else { Write-Host "Installed: $(Join-Path $bin 'rubolt.exe')" }
if(-not (Test-Path (Join-Path $bin 'rbcli.exe'))){ Write-Warning 'rbcli.exe not found in bin.' } else { Write-Host "Installed: $(Join-Path $bin 'rbcli.exe')" }

if($AddToPath){ Add-UserPath $bin }

Write-Host "[Installer] Done."