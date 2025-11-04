param(
  [string]$Prefix = "vendor"
)

$ErrorActionPreference = 'Stop'

$OUTDIR = Join-Path $Prefix 'bin'
if(-not (Test-Path $OUTDIR)){ New-Item -ItemType Directory -Path $OUTDIR | Out-Null }

function Get-Compiler {
  if(Get-Command cl -ErrorAction SilentlyContinue){ return 'cl' }
  elseif(Get-Command gcc -ErrorAction SilentlyContinue){ return 'gcc' }
  elseif(Get-Command clang -ErrorAction SilentlyContinue){ return 'clang' }
  else { return $null }
}

$cc = Get-Compiler
if(-not $cc){ Write-Warning 'No C compiler found (cl/gcc/clang)'; return }

$src = 'vendor/script_ops/script_ops.c'
$inc = 'vendor/script_ops'

if($cc -eq 'cl'){
  $out = Join-Path $OUTDIR 'script_ops.dll'
  $cmd = "cl /LD /O2 /I`"$inc`" `"$src`" /Fe:`"$out`""
  Write-Host "[script_ops] $cmd"
  cmd /c $cmd
} else {
  $out = Join-Path $OUTDIR 'script_ops.dll'
  $shared = '-shared'
  if($IsMacOS){ $out = Join-Path $OUTDIR 'script_ops.dylib' }
  elseif($IsLinux){ $out = Join-Path $OUTDIR 'script_ops.so' }
  $cmd = "${cc} ${shared} -O2 -fPIC -I`"$inc`" `"$src`" -o `"$out`""
  Write-Host "[script_ops] $cmd"
  bash -lc $cmd
}

Write-Host "Built: $out"