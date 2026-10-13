# Build / upload / monitor the Ether-Beast firmware with the Arduino IDE's bundled arduino-cli.
#   .\build.ps1                 compile and upload to COM5
#   .\build.ps1 build           compile only
#   .\build.ps1 monitor         serial monitor
#   .\build.ps1 upload -Port COM7
param(
    [ValidateSet('all', 'build', 'upload', 'monitor')]
    [string]$Action = 'all',
    [string]$Port = 'COM5'
)
$ErrorActionPreference = 'Stop'

$cli = (Get-Command arduino-cli -ErrorAction SilentlyContinue).Source
if (-not $cli) { $cli = "$env:LOCALAPPDATA\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe" }
if (-not (Test-Path $cli)) { throw "arduino-cli not found - install the Arduino IDE or arduino-cli" }

$sketch = Join-Path $PSScriptRoot 'firmware\EtherBeast'
$buildDir = Join-Path $PSScriptRoot 'build'
# Espressif core 2.0.17: the 3.x cores overflow IRAM with Bluetooth + Wi-Fi on the ESP32.
# Bluetooth + Wi-Fi + TLS needs more than the default 1.2MB app partition.
$fqbn = 'esp32:esp32:m5stack-coreink:PartitionScheme=no_ota'

# Generate secrets.h from .env (both are git-ignored).
$envFile = Join-Path $PSScriptRoot '.env'
if (Test-Path $envFile) {
    $vars = @{}
    foreach ($line in Get-Content $envFile) {
        if ($line -match '^\s*([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(.*)$') {
            $v = $matches[2].Trim()
            if ($v.Length -ge 2 -and (($v[0] -eq '"' -and $v[-1] -eq '"') -or ($v[0] -eq "'" -and $v[-1] -eq "'"))) { $v = $v.Substring(1, $v.Length - 2) }
            $vars[$matches[1]] = $v.Replace('\', '\\').Replace('"', '\"')
        }
    }
    foreach ($k in 'WIFI_HOME_SSID', 'WIFI_HOME_PASSWORD', 'GITHUB_PAT') {
        if (-not $vars[$k]) { throw ".env is missing $k" }
    }
    $secrets = @(
        '#pragma once',
        '// Generated from .env by build.ps1 - do not edit or commit.',
        "#define HOME_SSID     `"$($vars.WIFI_HOME_SSID)`"",
        "#define HOME_PASSWORD `"$($vars.WIFI_HOME_PASSWORD)`"",
        "#define GITHUB_TOKEN  `"$($vars.GITHUB_PAT)`""
    ) -join "`n"
    [IO.File]::WriteAllText("$sketch\secrets.h", $secrets + "`n")
}
if (-not (Test-Path "$sketch\secrets.h")) {
    throw "Missing secrets: create .env (WIFI_HOME_SSID, WIFI_HOME_PASSWORD, GITHUB_PAT) or firmware\EtherBeast\secrets.h"
}

& "$PSScriptRoot\tools\gen_sprites.ps1"

if ($Action -in 'all', 'build') {
    & $cli compile --fqbn $fqbn --build-path $buildDir $sketch
    if ($LASTEXITCODE) { exit $LASTEXITCODE }
}
if ($Action -in 'all', 'upload') {
    & $cli upload --fqbn $fqbn --input-dir $buildDir -p $Port $sketch
    if ($LASTEXITCODE) { exit $LASTEXITCODE }
}
if ($Action -eq 'monitor') {
    & $cli monitor -p $Port -c baudrate=115200
}
