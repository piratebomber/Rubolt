param(
  [string]$OutDir = "dist"
)

$ErrorActionPreference = 'Stop'

if(Test-Path $OutDir){ Remove-Item -Recurse -Force $OutDir }
New-Item -ItemType Directory -Path "$OutDir/bin" | Out-Null
New-Item -ItemType Directory -Path "$OutDir/lib" | Out-Null
New-Item -ItemType Directory -Path "$OutDir/share" | Out-Null

# Build if artifacts are missing
$needBuild = $false
if(-not (Test-Path 'src/rubolt.exe')){ $needBuild = $true }
if(-not (Test-Path 'rbcli.exe') -and -not (Test-Path 'cli/rbcli.exe')){ $needBuild = $true }
if($needBuild -and (Test-Path 'build_all.bat')){
  Write-Host '[Package] Building project via build_all.bat'
  cmd /c build_all.bat
}

# Bin
if(Test-Path 'src/rubolt.exe'){ Copy-Item 'src/rubolt.exe' "$OutDir/bin/" -Force }
if(Test-Path 'rbcli.exe'){ Copy-Item 'rbcli.exe' "$OutDir/bin/" -Force } elseif(Test-Path 'cli/rbcli.exe'){ Copy-Item 'cli/rbcli.exe' "$OutDir/bin/rbcli.exe" -Force }

# Lib content
foreach($d in 'StdLib','Objects','Modules'){
  if(Test-Path $d){ Copy-Item $d "$OutDir/lib/" -Recurse -Force }
}

# Share content
foreach($d in 'Docs','examples','vscode-rubolt','python','vendor'){
  if(Test-Path $d){ Copy-Item $d "$OutDir/share/" -Recurse -Force }
}

# Top-level docs
foreach($f in 'LICENSE','README.md'){
  if(Test-Path $f){ Copy-Item $f "$OutDir/share/" -Force }
}

Write-Host "[Package] dist prepared at $OutDir"