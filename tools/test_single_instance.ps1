param(
    [Parameter(Mandatory=$true)][string]$Exe,
    [Parameter(Mandatory=$true)][string]$Drivers,
    [Parameter(Mandatory=$true)][string]$Indexes,
    [Parameter(Mandatory=$true)][string]$Output
)
$ErrorActionPreference='Stop'
$Exe=[IO.Path]::GetFullPath($Exe)
$Output=[IO.Path]::GetFullPath($Output)
New-Item -ItemType Directory -Force -Path $Output | Out-Null
$session=[Guid]::NewGuid().ToString('N')
$reports=@((Join-Path $Output ($session+'-a.json')),(Join-Path $Output ($session+'-b.json')))
$taskProcesses=@()
foreach($report in $reports){
    $arguments='--scan-report "'+$report+'" --drivers "'+$Drivers+'" --indexes "'+$Indexes+'" --repeat-scan'
    $taskProcesses+=Start-Process -FilePath $Exe -ArgumentList $arguments -WindowStyle Hidden -PassThru
}
foreach($taskProcess in $taskProcesses){
    if(-not $taskProcess.WaitForExit(30000)){throw 'Read-only scan test exceeded its deadline.'}
    $taskProcess.Refresh()
}
$codes=@($taskProcesses | ForEach-Object {$_.ExitCode} | Sort-Object)
$written=@($reports | Where-Object {Test-Path -LiteralPath $_ -PathType Leaf})
if($codes.Count -ne 2 -or $codes[0] -ne 0 -or $codes[1] -ne 4 -or $written.Count -ne 1){
    throw ('Duplicate-launch test failed: exits '+($codes -join ',')+'; reports '+$written.Count)
}
$scan=Get-Content -LiteralPath $written[0] -Raw | ConvertFrom-Json
if($scan.error -or $scan.installationRequested){throw 'Unexpected scan result.'}
$result=@{passed=$true;exitCodes=$codes;reportsWritten=$written.Count;installationRequested=$false;devices=$scan.devices.Count}
$result | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $Output 'single-instance-result.json')
Write-Output 'Concurrent launch passed: one real scan completed; the other exited before starting a second engine.'
