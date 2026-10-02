param(
  [string]$AuditDir = "evoto-audit"
)
$ErrorActionPreference = 'Continue'
New-Item -ItemType Directory -Force -Path $AuditDir | Out-Null
$installer = Join-Path $AuditDir 'Evoto-Official-Setup.exe'
if (-not (Test-Path $installer)) { throw "Installer not found: $installer" }

$report = [ordered]@{}
$report.AuditedAt = (Get-Date).ToUniversalTime().ToString('o')
$report.InstallerPath = (Resolve-Path $installer).Path
$item = Get-Item $installer
$report.InstallerSize = $item.Length
$report.SHA256 = (Get-FileHash $installer -Algorithm SHA256).Hash
$report.VersionInfo = [ordered]@{
  FileVersion = $item.VersionInfo.FileVersion
  ProductVersion = $item.VersionInfo.ProductVersion
  CompanyName = $item.VersionInfo.CompanyName
  ProductName = $item.VersionInfo.ProductName
  FileDescription = $item.VersionInfo.FileDescription
  OriginalFilename = $item.VersionInfo.OriginalFilename
}
$sig = Get-AuthenticodeSignature $installer
$report.Signature = [ordered]@{
  Status = [string]$sig.Status
  StatusMessage = $sig.StatusMessage
  Subject = if($sig.SignerCertificate){$sig.SignerCertificate.Subject}else{$null}
  Issuer = if($sig.SignerCertificate){$sig.SignerCertificate.Issuer}else{$null}
  Thumbprint = if($sig.SignerCertificate){$sig.SignerCertificate.Thumbprint}else{$null}
}

# Archive/installer metadata only. Do not extract or publish model weights or proprietary assets.
$seven = Get-Command 7z.exe -ErrorAction SilentlyContinue
if ($seven) {
  try {
    & $seven.Source l $installer | Out-File (Join-Path $AuditDir 'installer-7z-list.txt') -Encoding utf8
    $report.SevenZipListing = 'installer-7z-list.txt'
  } catch {}
}

# Try common silent installation switches on the ephemeral Windows runner.
# We do not bypass sign-in, licensing, or any protected feature.
$attempts = @(
  @('/S'),
  @('/silent'),
  @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART')
)
$installAttempts = @()
$installed = $false
foreach($args in $attempts){
  if($installed){break}
  try {
    $p = Start-Process -FilePath $installer -ArgumentList $args -PassThru
    $exited = $p.WaitForExit(120000)
    if(-not $exited){ try{$p.Kill()}catch{} }
    Start-Sleep -Seconds 4
    $roots = @(
      "$env:ProgramFiles\Evoto",
      "${env:ProgramFiles(x86)}\Evoto",
      "$env:LOCALAPPDATA\Programs\Evoto",
      "$env:LOCALAPPDATA\Evoto",
      "$env:APPDATA\Evoto"
    ) | Where-Object { $_ -and (Test-Path $_) }
    $installAttempts += [ordered]@{Args=($args -join ' ');Exited=$exited;ExitCode=if($exited){$p.ExitCode}else{$null};Roots=$roots}
    if($roots.Count -gt 0){$installed=$true;break}
  } catch {
    $installAttempts += [ordered]@{Args=($args -join ' ');Error=$_.Exception.Message}
  }
}
$report.InstallAttempts = $installAttempts

# Broader recent install search if normal roots were not found.
$searchRoots = @($env:ProgramFiles, ${env:ProgramFiles(x86)}, $env:LOCALAPPDATA) | Where-Object { $_ -and (Test-Path $_) }
$evotoDirs = @()
foreach($root in $searchRoots){
  try {
    $evotoDirs += Get-ChildItem -Path $root -Directory -ErrorAction SilentlyContinue | Where-Object { $_.Name -match 'Evoto' } | Select-Object -ExpandProperty FullName
  } catch {}
}
$evotoDirs = $evotoDirs | Sort-Object -Unique
$report.InstallRoots = $evotoDirs

$filesSummary = @()
$mainExe = $null
foreach($root in $evotoDirs){
  try {
    $all = Get-ChildItem $root -Recurse -File -ErrorAction SilentlyContinue
    $byExt = $all | Group-Object Extension | Sort-Object Count -Descending | Select-Object -First 30 @{n='Extension';e={$_.Name}},Count
    $largest = $all | Sort-Object Length -Descending | Select-Object -First 40 FullName,Length,Extension
    $exeCandidates = $all | Where-Object { $_.Extension -eq '.exe' -and $_.Name -match 'Evoto' } | Sort-Object Length -Descending
    if(-not $mainExe -and $exeCandidates){$mainExe=$exeCandidates[0].FullName}
    $filesSummary += [ordered]@{
      Root=$root
      FileCount=$all.Count
      ByExtension=$byExt
      LargestFiles=$largest
      Executables=$exeCandidates | Select-Object -First 20 FullName,Length,@{n='FileVersion';e={$_.VersionInfo.FileVersion}},@{n='ProductVersion';e={$_.VersionInfo.ProductVersion}}
    }
  } catch {}
}
$report.InstallFileSummary = $filesSummary
$report.MainExecutable = $mainExe

# Launch only to observe public UI/window metadata. No license bypass or account automation.
$ui = [ordered]@{Launched=$false;ProcessInfo=@();AutomationControls=@();Screenshot=$null}
if($mainExe -and (Test-Path $mainExe)){
  try{
    $proc=Start-Process -FilePath $mainExe -PassThru
    $ui.Launched=$true
    Start-Sleep -Seconds 25
    $procs = Get-Process | Where-Object { $_.ProcessName -match 'Evoto' }
    $ui.ProcessInfo = $procs | Select-Object Id,ProcessName,MainWindowTitle,Path,CPU,WorkingSet64

    # Public UI Automation tree: names/control types only, no protected internals.
    try {
      Add-Type -AssemblyName UIAutomationClient
      Add-Type -AssemblyName UIAutomationTypes
      $controls = @()
      foreach($ep in $procs){
        if($ep.MainWindowHandle -ne 0){
          $rootEl=[System.Windows.Automation.AutomationElement]::FromHandle($ep.MainWindowHandle)
          if($rootEl){
            $walker=[System.Windows.Automation.TreeWalker]::ControlViewWalker
            function Walk-Node($node,[int]$depth){
              if(-not $node -or $depth -gt 8 -or $script:controls.Count -gt 3000){return}
              try{
                $script:controls += [ordered]@{
                  Depth=$depth
                  Name=$node.Current.Name
                  AutomationId=$node.Current.AutomationId
                  ControlType=$node.Current.ControlType.ProgrammaticName
                  ClassName=$node.Current.ClassName
                  IsEnabled=$node.Current.IsEnabled
                  IsOffscreen=$node.Current.IsOffscreen
                }
              }catch{}
              try{
                $child=$walker.GetFirstChild($node)
                while($child){Walk-Node $child ($depth+1);$child=$walker.GetNextSibling($child)}
              }catch{}
            }
            $script:controls=@()
            Walk-Node $rootEl 0
            $controls += $script:controls
          }
        }
      }
      $ui.AutomationControls=$controls
      $controls | ConvertTo-Json -Depth 6 | Out-File (Join-Path $AuditDir 'evoto-ui-automation.json') -Encoding utf8
    }catch{}

    # Best-effort desktop screenshot on runner; may not be available on non-interactive CI.
    try{
      Add-Type -AssemblyName System.Drawing
      Add-Type -AssemblyName System.Windows.Forms
      $bounds=[System.Windows.Forms.SystemInformation]::VirtualScreen
      if($bounds.Width -gt 0 -and $bounds.Height -gt 0){
        $bmp=New-Object System.Drawing.Bitmap $bounds.Width,$bounds.Height
        $g=[System.Drawing.Graphics]::FromImage($bmp)
        $g.CopyFromScreen($bounds.Left,$bounds.Top,0,0,$bounds.Size)
        $shot=Join-Path $AuditDir 'evoto-runner-desktop.png'
        $bmp.Save($shot,[System.Drawing.Imaging.ImageFormat]::Png)
        $g.Dispose();$bmp.Dispose()
        if((Get-Item $shot).Length -gt 10000){$ui.Screenshot='evoto-runner-desktop.png'}
      }
    }catch{}

    foreach($ep in $procs){try{$ep.CloseMainWindow()|Out-Null;Start-Sleep -Milliseconds 500;if(-not $ep.HasExited){$ep.Kill()}}catch{}}
  }catch{
    $ui.Error=$_.Exception.Message
  }
}
$report.UIObservation=$ui

$report | ConvertTo-Json -Depth 12 | Out-File (Join-Path $AuditDir 'evoto-installed-audit.json') -Encoding utf8
$report | ConvertTo-Json -Depth 12
