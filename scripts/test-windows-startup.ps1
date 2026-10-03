$ErrorActionPreference = 'Stop'
$env:PATH = (($env:PATH -split ';') | Where-Object { $_ -notmatch 'mingw64[\\/]bin' -and $_ -notmatch 'hamlib-prefix[\\/]bin' }) -join ';'
$env:QT_QPA_PLATFORM = 'offscreen'
$env:QT_DEBUG_PLUGINS = '1'
$info = New-Object System.Diagnostics.ProcessStartInfo
$info.FileName = (Resolve-Path 'build/windows-package/bin/Aurora-JTTY.exe').Path
$info.Arguments = '--startup-smoke-test --rig-name CI-STARTUP'
$info.UseShellExecute = $false
$info.RedirectStandardOutput = $true
$info.RedirectStandardError = $true
$process = New-Object System.Diagnostics.Process
$process.StartInfo = $info
$process.Start() | Out-Null
$stdout = $process.StandardOutput.ReadToEndAsync()
$stderr = $process.StandardError.ReadToEndAsync()
$finished = $process.WaitForExit(60000)
if (-not $finished) {
    "Timed out: PID $($process.Id), window '$($process.MainWindowTitle)'" | Set-Content launch.out
    Get-Process | Where-Object { $_.ProcessName -match 'Aurora|wsjtx' } | Select-Object Id,ProcessName,MainWindowTitle | Out-String | Add-Content launch.out
    $process.Kill()
    $process.WaitForExit()
}
$stdout.Result | Add-Content launch.out
$stderr.Result | Add-Content launch.out
Get-Content launch.out
if (-not $finished) { throw 'Packaged application did not finish its startup test in 60 seconds' }
if ($process.ExitCode -ne 0) { throw "Startup test exited with code $($process.ExitCode)" }
if ($stdout.Result -notmatch 'startup smoke test passed') { throw 'Startup success marker missing' }
Write-Host 'Packaged Windows startup test passed with development DLL paths removed.'
