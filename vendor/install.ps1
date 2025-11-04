param(
  [switch]$Analyze,
  [string]$Prefix = "vendor",
  [string]$Manifest = "vendor/manifest.json",
  [string]$Python = "python"
)

$ErrorActionPreference = 'Stop'

$include = Join-Path $Prefix 'include'
$lib = Join-Path $Prefix 'lib'
$bin = Join-Path $Prefix 'bin'
$src = Join-Path $Prefix 'src'
$py = Join-Path $Prefix 'python'

foreach($d in @($include,$lib,$bin,$src,$py)){
  if(-not (Test-Path $d)){ New-Item -ItemType Directory -Path $d | Out-Null }
}

if($Analyze){
  & $Python vendor/analyze_project.py --manifest $Manifest --out (Join-Path $Prefix 'generated_manifest.json')
  $Manifest = Join-Path $Prefix 'generated_manifest.json'
}

if(-not (Test-Path $Manifest)){ throw "Manifest not found: $Manifest" }

function Download-File($Url, $Out){
  Write-Host "[vendor] Downloading $Url -> $Out"
  try {
    Invoke-WebRequest -Uri $Url -OutFile $Out -UseBasicParsing
  } catch {
    try { curl -L $Url -o $Out } catch { throw "Failed to download $Url" }
  }
}

function Extract-Archive($File, $Dest){
  if(-not (Test-Path $Dest)){ New-Item -ItemType Directory -Path $Dest | Out-Null }
  if($File -match '\.zip$'){
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    [System.IO.Compression.ZipFile]::ExtractToDirectory($File, $Dest)
  } elseif($File -match '\.(tar\.gz|tgz|tar\.bz2|tbz2)$'){
    if(Get-Command tar -ErrorAction SilentlyContinue){ tar -xf $File -C $Dest }
    else { throw "tar not available to extract $File" }
  } else { throw "Unknown archive type: $File" }
}

# Read manifest JSON
$manifestObj = Get-Content $Manifest -Raw | ConvertFrom-Json

# Install Python packages
if($manifestObj.python){
  foreach($pkg in $manifestObj.python){
    $name = $pkg.name
    $ver = $pkg.version
    $spec = if($ver){ "$name$ver" } else { $name }
    Write-Host "[vendor] Installing Python package: $spec"
    & $Python -m pip install -t $py $spec
  }
}

# Load registry
$registryPath = 'vendor/registry.json'
$registry = $null
if(Test-Path $registryPath){ $registry = Get-Content $registryPath -Raw | ConvertFrom-Json }

# Resolve includes -> vendor packages
$vendorNames = @()
if($manifestObj.includes -and $registry){
  foreach($hdr in $manifestObj.includes){
    $pkg = $registry.header_map.$hdr
    if($pkg){ $vendorNames += $pkg }
  }
}
# Explicit vendors
if($manifestObj.vendors){
  foreach($v in $manifestObj.vendors){ $vendorNames += $v.name }
}
$vendorNames = $vendorNames | Sort-Object -Unique

# OS id
$osId = 'windows'
if($IsWindows){ $osId = 'windows' } elseif($IsLinux){ $osId = 'linux' } elseif($IsMacOS){ $osId = 'macos' }

# Build/Install each vendor
foreach($v in $vendorNames){
  if($v -eq 'script_ops'){
    Write-Host "[vendor] Building script_ops..."
    pwsh -File vendor/script_ops/build.ps1 -Prefix $Prefix
    continue
  }
  if(-not $registry){ continue }
  Write-Host "[vendor] Installing vendor: $v"
  $asset = $registry.packages.$v.assets.$osId
  if($asset -and $asset.url){
    $archive = Join-Path $src ("$v-archive")
    Download-File $asset.url $archive
    $dest = Join-Path $src $v
    Extract-Archive $archive $dest
    # Opportunistic copies
    Get-ChildItem -Recurse -Directory $dest -Filter include -ErrorAction SilentlyContinue | ForEach-Object { Copy-Item -Recurse -Force (Join-Path $_.FullName '*') $include }
    Get-ChildItem -Recurse -Directory $dest -Filter lib -ErrorAction SilentlyContinue | ForEach-Object { Copy-Item -Recurse -Force (Join-Path $_.FullName '*') $lib }
    Get-ChildItem -Recurse -Directory $dest -Filter bin -ErrorAction SilentlyContinue | ForEach-Object { Copy-Item -Recurse -Force (Join-Path $_.FullName '*') $bin }
    continue
  }
  # Package manager fallback
  $cmds = $registry.packages.$v.pkgman.$osId
  if($cmds){
    foreach($c in $cmds){
      Write-Host "[vendor] Running: $c"
      try { & powershell -NoProfile -Command $c } catch { Write-Warning "Failed: $c" }
    }
  } else { Write-Warning "No install method for $v on $osId" }
}

# Generate env script
$envPs1 = @"
# Source with: . $Prefix/env.ps1
$env:RUBOLT_VENDOR_PREFIX = ""$Prefix""
$env:RUBOLT_VENDOR_INCLUDE = ""$include""
$env:RUBOLT_VENDOR_LIB = ""$lib""
$env:RUBOLT_VENDOR_BIN = ""$bin""
$env:PYTHONPATH = ""$py;"" + (
  if($env:PYTHONPATH){ $env:PYTHONPATH } else { '' }
)
$env:PATH = ""$bin;"" + $env:PATH
"@
Set-Content -Path (Join-Path $Prefix 'env.ps1') -Value $envPs1 -Encoding UTF8

Write-Host "[vendor] Done. Activate with: . $Prefix/env.ps1"
