param(
  [Parameter(Mandatory=$true)]
  [string]$InstallRoot,
  [string]$OutputDir = "$PSScriptRoot\evoto-audit-output"
)

$ErrorActionPreference = 'SilentlyContinue'
$InstallRoot = (Resolve-Path $InstallRoot).Path
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

function Write-Utf8([string]$Path,[object]$Value){
  $Value | Out-File -FilePath $Path -Encoding utf8 -Width 4096
}

# 1) Basic machine info (no serial numbers, no user account data)
$cpu = Get-CimInstance Win32_Processor | Select-Object Name,NumberOfCores,NumberOfLogicalProcessors
$gpu = Get-CimInstance Win32_VideoController | Select-Object Name,DriverVersion,AdapterRAM
$os  = Get-CimInstance Win32_OperatingSystem | Select-Object Caption,Version,OSArchitecture,@{N='RAM_GB';E={[math]::Round($_.TotalVisibleMemorySize/1MB,1)}}
@{
  audit_time = (Get-Date).ToString('s')
  install_root = $InstallRoot
  cpu = $cpu
  gpu = $gpu
  os = $os
} | ConvertTo-Json -Depth 6 | Set-Content "$OutputDir\system.json" -Encoding utf8

# 2) Complete file inventory
$files = Get-ChildItem -LiteralPath $InstallRoot -Recurse -File -Force
$inventory = foreach($f in $files){
  [pscustomobject]@{
    RelativePath = $f.FullName.Substring($InstallRoot.Length).TrimStart('\')
    Extension = $f.Extension.ToLowerInvariant()
    SizeBytes = $f.Length
    LastWriteTime = $f.LastWriteTime.ToString('s')
  }
}
$inventory | Export-Csv "$OutputDir\file_inventory.csv" -NoTypeInformation -Encoding utf8

# 3) Binary version metadata; no binary contents copied
$binaries = foreach($f in $files | Where-Object {$_.Extension -in '.exe','.dll'}){
  $v = [System.Diagnostics.FileVersionInfo]::GetVersionInfo($f.FullName)
  [pscustomobject]@{
    RelativePath = $f.FullName.Substring($InstallRoot.Length).TrimStart('\')
    SizeBytes = $f.Length
    CompanyName = $v.CompanyName
    ProductName = $v.ProductName
    FileDescription = $v.FileDescription
    FileVersion = $v.FileVersion
    ProductVersion = $v.ProductVersion
    OriginalFilename = $v.OriginalFilename
  }
}
$binaries | Export-Csv "$OutputDir\binary_versions.csv" -NoTypeInformation -Encoding utf8

# 4) Model/runtime candidates by filename/extension only
$modelExt = '.onnx','.ort','.engine','.trt','.plan','.pt','.pth','.ckpt','.safetensors','.tflite','.bin','.param','.weights','.model','.ncnn','.mlmodel','.pb'
$modelCandidates = foreach($f in $files){
  $name = $f.Name.ToLowerInvariant()
  $path = $f.FullName.ToLowerInvariant()
  if(($modelExt -contains $f.Extension.ToLowerInvariant()) -or $path -match 'model|weights|network|checkpoint|onnx|tensor|cuda|directml|openvino|ncnn|mnn'){
    [pscustomobject]@{
      RelativePath = $f.FullName.Substring($InstallRoot.Length).TrimStart('\')
      Extension = $f.Extension.ToLowerInvariant()
      SizeBytes = $f.Length
    }
  }
}
$modelCandidates | Sort-Object RelativePath | Export-Csv "$OutputDir\model_runtime_candidates.csv" -NoTypeInformation -Encoding utf8

# 5) Runtime/engine fingerprint from file names
$runtimeKeywords = 'cuda','cudnn','tensorrt','nvinfer','onnxruntime','directml','dml','openvino','torch','pytorch','opencv','qt6','qt5','chromium','cef','ffmpeg','vulkan','dx12','d3d12','mkl','openmp','ncnn','mnn','tensorflow'
$hits = foreach($k in $runtimeKeywords){
  $matches = $files | Where-Object { $_.Name.ToLowerInvariant() -like "*$k*" } | Select-Object -First 30
  foreach($m in $matches){
    [pscustomobject]@{Keyword=$k;RelativePath=$m.FullName.Substring($InstallRoot.Length).TrimStart('\');SizeBytes=$m.Length}
  }
}
$hits | Export-Csv "$OutputDir\runtime_fingerprint.csv" -NoTypeInformation -Encoding utf8

# 6) Hash only small manifests/configs and application binaries; avoid hashing giant model files
$hashTargets = $files | Where-Object {
  ($_.Extension -in '.exe','.dll','.json','.ini','.yaml','.yml','.toml','.xml','.manifest') -and $_.Length -lt 100MB
}
$hashes = foreach($f in $hashTargets){
  $h = Get-FileHash -LiteralPath $f.FullName -Algorithm SHA256
  [pscustomobject]@{RelativePath=$f.FullName.Substring($InstallRoot.Length).TrimStart('\');SHA256=$h.Hash;SizeBytes=$f.Length}
}
$hashes | Export-Csv "$OutputDir\sha256_small_files.csv" -NoTypeInformation -Encoding utf8

# 7) Collect only safe text manifests/configs that appear application-level and are small.
#    Skip anything with obvious token/account/session/cookie names.
$safeTextDir = Join-Path $OutputDir 'safe_text_configs'
New-Item -ItemType Directory -Force -Path $safeTextDir | Out-Null
$deny = 'token|cookie|session|account|credential|password|secret|oauth|login|user|license|activation'
$textCandidates = $files | Where-Object {
  $_.Extension -in '.json','.ini','.yaml','.yml','.toml','.xml','.manifest','.txt' -and $_.Length -lt 2MB -and $_.Name.ToLowerInvariant() -notmatch $deny
}
foreach($f in $textCandidates){
  $rel = $f.FullName.Substring($InstallRoot.Length).TrimStart('\')
  $safe = ($rel -replace '[\\/:*?"<>|]','__')
  try { Copy-Item -LiteralPath $f.FullName -Destination (Join-Path $safeTextDir $safe) -Force } catch {}
}

# 8) Uninstall registration only
$uninstallRoots = @(
  'HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\*',
  'HKLM:\Software\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\*',
  'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\*'
)
$uninstall = foreach($r in $uninstallRoots){
  Get-ItemProperty $r | Where-Object {$_.DisplayName -match 'Evoto'} | Select-Object DisplayName,DisplayVersion,Publisher,InstallLocation,UninstallString
}
$uninstall | ConvertTo-Json -Depth 4 | Set-Content "$OutputDir\uninstall_info.json" -Encoding utf8

# 9) Summary
$summary = [pscustomobject]@{
  FileCount = $files.Count
  TotalBytes = ($files | Measure-Object Length -Sum).Sum
  ExeCount = ($files | Where-Object Extension -eq '.exe').Count
  DllCount = ($files | Where-Object Extension -eq '.dll').Count
  ModelCandidateCount = $modelCandidates.Count
  ConfigCopiedCount = $textCandidates.Count
}
$summary | ConvertTo-Json | Set-Content "$OutputDir\summary.json" -Encoding utf8

# 10) Zip report; this contains metadata/config text only, not Evoto binaries or model weights.
$zipPath = Join-Path (Split-Path $OutputDir -Parent) 'Evoto_Audit_Report.zip'
if(Test-Path $zipPath){Remove-Item $zipPath -Force}
Compress-Archive -Path "$OutputDir\*" -DestinationPath $zipPath -CompressionLevel Optimal

Write-Host "DONE"
Write-Host "Report: $zipPath"
Write-Host "Upload Evoto_Audit_Report.zip for analysis."
