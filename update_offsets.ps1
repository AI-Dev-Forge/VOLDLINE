# =====================================================================
#  CS2 Offset Auto-Updater
#  Fetches latest verified offsets from a2x/cs2-dumper output
# =====================================================================

$mainCpp = "$PSScriptRoot\VOLD-LINE\main.cpp"

Write-Host "====================================================" -ForegroundColor Cyan
Write-Host "  CS2 ESP Offset Auto-Updater" -ForegroundColor Cyan
Write-Host "====================================================" -ForegroundColor Cyan

[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
$wc = New-Object System.Net.WebClient
$wc.Headers.Add("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64)")

Write-Host "[*] Downloading offsets.hpp..." -ForegroundColor Yellow
try {
    $offsetsText = $wc.DownloadString('https://raw.githubusercontent.com/sezzyaep/CS2-OFFSETS/refs/heads/main/offsets.hpp')
}
catch {
    Write-Host "[-] Failed to download offsets.hpp: $($_.Exception.Message)" -ForegroundColor Red
    return
}

Write-Host "[*] Downloading client_dll.hpp..." -ForegroundColor Yellow
try {
    $clientText = $wc.DownloadString('https://raw.githubusercontent.com/sezzyaep/CS2-OFFSETS/refs/heads/main/client_dll.hpp')
}
catch {
    Write-Host "[-] Failed to download client_dll.hpp: $($_.Exception.Message)" -ForegroundColor Red
    return
}

function ExtractHex($text, $pattern) {
    if ($text -match $pattern) {
        return $matches[1]
    }
    return $null
}

$dwEntityList = ExtractHex $offsetsText 'constexpr std::ptrdiff_t dwEntityList = (0x[0-9A-Fa-f]+);'
$dwViewMatrix = ExtractHex $offsetsText 'constexpr std::ptrdiff_t dwViewMatrix = (0x[0-9A-Fa-f]+);'
$dwLocalPlayerPawn = ExtractHex $offsetsText 'constexpr std::ptrdiff_t dwLocalPlayerPawn = (0x[0-9A-Fa-f]+);'
$dwViewAngles = ExtractHex $offsetsText 'constexpr std::ptrdiff_t dwViewAngles = (0x[0-9A-Fa-f]+);'

$m_hPlayerPawn = ExtractHex $clientText 'constexpr std::ptrdiff_t m_hPlayerPawn = (0x[0-9A-Fa-f]+);'
$m_iHealth = ExtractHex $clientText 'constexpr std::ptrdiff_t m_iHealth = (0x[0-9A-Fa-f]+);'
$m_lifeState = ExtractHex $clientText 'constexpr std::ptrdiff_t m_lifeState = (0x[0-9A-Fa-f]+);'
$m_iTeamNum = ExtractHex $clientText 'constexpr std::ptrdiff_t m_iTeamNum = (0x[0-9A-Fa-f]+);'
$m_vOldOrigin = ExtractHex $clientText 'constexpr std::ptrdiff_t m_vOldOrigin = (0x[0-9A-Fa-f]+);'
$m_vecViewOffset = ExtractHex $clientText 'constexpr std::ptrdiff_t m_vecViewOffset = (0x[0-9A-Fa-f]+);'

Write-Host "`nFetched Values:" -ForegroundColor Green
Write-Host "dwEntityList      = $dwEntityList"
Write-Host "dwViewMatrix      = $dwViewMatrix"
Write-Host "dwLocalPlayerPawn = $dwLocalPlayerPawn"
Write-Host "dwViewAngles      = $dwViewAngles"
Write-Host "m_hPlayerPawn     = $m_hPlayerPawn"
Write-Host "m_iHealth         = $m_iHealth"
Write-Host "m_lifeState       = $m_lifeState"
Write-Host "m_iTeamNum        = $m_iTeamNum"
Write-Host "m_vOldOrigin      = $m_vOldOrigin"
Write-Host "m_vecViewOffset   = $m_vecViewOffset"

$content = Get-Content $mainCpp -Raw
$original = $content

function UpdateOffset($cnt, $name, $val) {
    if (-not $val) { return $cnt }
    $p = "(constexpr uintptr_t $name\s*=\s*)(0x[0-9A-Fa-f]+)(;)"
    return [regex]::Replace($cnt, $p, "`${1}$val`${3}")
}

$content = UpdateOffset $content 'dwEntityList' $dwEntityList
$content = UpdateOffset $content 'dwViewMatrix' $dwViewMatrix
$content = UpdateOffset $content 'dwLocalPlayerPawn' $dwLocalPlayerPawn
$content = UpdateOffset $content 'dwViewAngles' $dwViewAngles
$content = UpdateOffset $content 'm_hPlayerPawn' $m_hPlayerPawn
$content = UpdateOffset $content 'm_iHealth' $m_iHealth
$content = UpdateOffset $content 'm_lifeState' $m_lifeState
$content = UpdateOffset $content 'm_iTeamNum' $m_iTeamNum
$content = UpdateOffset $content 'm_vOldOrigin' $m_vOldOrigin
$content = UpdateOffset $content 'm_vecViewOffset' $m_vecViewOffset

if (-not $dwEntityList -or -not $dwViewMatrix) {
    Write-Host "`n[-] Offsets could not be extracted. main.cpp was not modified." -ForegroundColor Red
    return
}

if ($content -ne $original) {
    Set-Content $mainCpp $content -NoNewline
    Write-Host "`n[+] main.cpp successfully updated!" -ForegroundColor Green
}
else {
    Write-Host "`n[=] main.cpp is already matching latest offsets." -ForegroundColor Cyan
}
