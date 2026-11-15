@echo off
setlocal

powershell.exe -NoProfile -ExecutionPolicy Bypass -Command ^
  "$ErrorActionPreference = 'Stop';" ^
  "Write-Host '=== GPU adapter candidates ===' -ForegroundColor Cyan;" ^
  "$controllers = Get-CimInstance Win32_VideoController;" ^
  "foreach ($gpu in $controllers) {" ^
  "  Write-Host ('Name: ' + $gpu.Name);" ^
  "  Write-Host ('PNPDeviceID: ' + $gpu.PNPDeviceID);" ^
  "  if ($gpu.PNPDeviceID -match 'VEN_([0-9A-Fa-f]{4}).*DEV_([0-9A-Fa-f]{4}).*SUBSYS_([0-9A-Fa-f]{8})') {" ^
  "    $id = 'HighPerfAdapter=' + $matches[1].ToUpper() + '&' + $matches[2].ToUpper() + '&' + $matches[3].ToUpper() + ';';" ^
  "    Write-Host ('adapter_id: ' + $id) -ForegroundColor Green;" ^
  "  } else {" ^
  "    Write-Host 'adapter_id: unable to derive from PNPDeviceID' -ForegroundColor Yellow;" ^
  "  }" ^
  "  Write-Host '';" ^
  "}" ^
  "Write-Host '=== Current DirectXUserGlobalSettings ===' -ForegroundColor Cyan;" ^
  "$path = 'HKCU:\Software\Microsoft\DirectX\UserGpuPreferences';" ^
  "if (Test-Path $path) {" ^
  "  (Get-ItemProperty -Path $path -Name DirectXUserGlobalSettings -ErrorAction SilentlyContinue).DirectXUserGlobalSettings;" ^
  "} else {" ^
  "  Write-Host 'Registry value is not present.' -ForegroundColor Yellow;" ^
  "}"

if errorlevel 1 (
    echo Failed to query GPU information.
    pause
    exit /b 1
)

pause
endlocal
