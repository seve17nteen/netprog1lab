# Автоматическая проверка lab1.exe: успешные сценарии и граничные случаи.
# Запуск:  powershell -ExecutionPolicy Bypass -File .\tests.ps1

$ErrorActionPreference = 'Stop'
$exe = Join-Path $PSScriptRoot 'lab1.exe'

if (-not (Test-Path $exe)) {
    Write-Host 'Сначала соберите программу: build.bat' -ForegroundColor Red
    exit 1
}

$failed = 0
$total = 0
$utf8 = New-Object System.Text.UTF8Encoding($false)

function Invoke-Program {
    param([string[]]$Arguments)

    $info = New-Object System.Diagnostics.ProcessStartInfo
    $info.FileName = $exe
    $info.Arguments = ($Arguments | ForEach-Object {
        if ($_ -eq '') { '""' } else { '"' + $_ + '"' }
    }) -join ' '
    $info.UseShellExecute = $false
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    $info.StandardOutputEncoding = $utf8
    $info.StandardErrorEncoding = $utf8
    $info.CreateNoWindow = $true

    $process = [System.Diagnostics.Process]::Start($info)
    $stdout = $process.StandardOutput.ReadToEnd()
    $stderr = $process.StandardError.ReadToEnd()
    $process.WaitForExit()

    return @{
        Code   = $process.ExitCode
        Output = $stdout + $stderr
    }
}

function Invoke-Case {
    param(
        [string]$Name,
        [string[]]$Arguments,
        [int[]]$ExpectedCode,
        [string]$ExpectedPattern = ''
    )

    $script:total++
    $result = Invoke-Program -Arguments $Arguments
    $ok = $ExpectedCode -contains $result.Code

    if ($ok -and $ExpectedPattern -ne '') {
        $ok = $result.Output -match $ExpectedPattern
    }

    if ($ok) {
        Write-Host ("[ OK ] {0,-46} код={1}" -f $Name, $result.Code) -ForegroundColor Green
    } else {
        $script:failed++
        Write-Host ("[FAIL] {0,-46} код={1}, ожидался {2}" -f $Name, $result.Code,
            ($ExpectedCode -join '/')) -ForegroundColor Red
        if ($ExpectedPattern -ne '' -and ($ExpectedCode -contains $result.Code)) {
            Write-Host ("       нет совпадения с шаблоном: {0}" -f $ExpectedPattern) `
                -ForegroundColor Yellow
        }
        Write-Host '       --- вывод ---'
        $result.Output -split "`r?`n" | ForEach-Object { Write-Host ("       " + $_) }
    }
}

Write-Host '=== Успешные сценарии (ожидается код 0) ===' -ForegroundColor Cyan
Invoke-Case 'IPv4/TCP без --bind' @('inet', 'stream', '127.0.0.1', '8080') 0 'SOCK_STREAM'
Invoke-Case 'IPv4/TCP с --bind' @('inet', 'stream', '127.0.0.1', '8080', '--bind') 0 'bind\(\): успешно'
Invoke-Case 'IPv4/UDP с --bind' @('inet', 'dgram', '127.0.0.1', '9090', '--bind') 0 'SOCK_DGRAM'
Invoke-Case 'IPv6/UDP с --bind' @('inet6', 'dgram', '::1', '5555', '--bind') 0 'SOCK_DGRAM'
Invoke-Case 'IPv4/stream нулевой порт (0)' @('inet', 'stream', '0.0.0.0', '0', '--bind') 0 'getsockname'
Invoke-Case 'порт 65535 (максимум)' @('inet', 'udp', '127.0.0.1', '65535') 0 'SO_TYPE'
Invoke-Case 'алиасы tcp/ipv4' @('ipv4', 'tcp', '127.0.0.1', '8081') 0 'SOCK_STREAM'
Invoke-Case 'алиасы udp/af_inet6' @('af_inet6', 'udp', '::1', '8082') 0 'SOCK_DGRAM'
Invoke-Case 'справка -h' @('-h') 0 'Использование'

Write-Host ''
Write-Host '=== Аргументы командной строки (ожидается код 1) ===' -ForegroundColor Cyan
Invoke-Case 'нет аргументов' @() 1 'недостаточно аргументов'
Invoke-Case 'только 2 аргумента' @('inet', 'stream') 1 'недостаточно аргументов'
Invoke-Case 'лишний аргумент' @('inet', 'stream', '127.0.0.1', '80', '--bind', 'extra') 1 'слишком много'
Invoke-Case 'неизвестный флаг' @('inet', 'stream', '127.0.0.1', '80', '--verbose') 1 'неизвестный аргумент'

Write-Host ''
Write-Host '=== Граничные случаи значений (ожидается код 2) ===' -ForegroundColor Cyan
Invoke-Case 'неверный IP: 256.1.1.1' @('inet', 'stream', '256.1.1.1', '80') 2 'неверный IP'
Invoke-Case 'неверный IP: 1.2.3' @('inet', 'stream', '1.2.3', '80') 2 'неверный IP'
Invoke-Case 'имя хоста вместо IP' @('inet', 'stream', 'localhost', '80') 2 'неверный IP'
Invoke-Case 'пустой IP' @('inet', 'stream', '', '80') 2 'неверный IP'
Invoke-Case 'IPv6-адрес при domain=inet' @('inet', 'stream', '::1', '80') 2 'неверный IP'
Invoke-Case 'IPv4-адрес при domain=inet6' @('inet6', 'stream', '127.0.0.1', '80') 2 'неверный IP'
Invoke-Case 'порт 65536 (> 65535)' @('inet', 'stream', '127.0.0.1', '65536') 2 'неверный порт'
Invoke-Case 'порт 99999999999999999999' @('inet', 'stream', '127.0.0.1', '99999999999999999999') 2 'неверный порт'
Invoke-Case 'отрицательный порт' @('inet', 'stream', '127.0.0.1', '-1') 2 'неверный порт'
Invoke-Case 'порт с буквами' @('inet', 'stream', '127.0.0.1', '80abc') 2 'неверный порт'
Invoke-Case 'пустой порт' @('inet', 'stream', '127.0.0.1', '') 2 'неверный порт'
Invoke-Case 'неверный domain' @('inet4', 'stream', '127.0.0.1', '80') 2 'неверное значение <domain>'
Invoke-Case 'неверный type сокета' @('inet', 'badtype', '127.0.0.1', '80') 2 'неверное значение <type>'

Write-Host ''
Write-Host '=== Ошибки системных вызовов ===' -ForegroundColor Cyan
# SOCK_RAW: без прав администратора socket() даёт WSAEACCES (код 3),
# с правами администратора сокет создаётся успешно (код 0).
Invoke-Case 'SOCK_RAW (права администратора)' @('inet', 'raw', '127.0.0.1', '80') 0, 3 ` 'socket|SO_TYPE'

Write-Host ''
Write-Host '=== Занятый порт: второй bind на тот же порт ===' -ForegroundColor Cyan
$listener = [System.Net.Sockets.TcpListener]::new([System.Net.IPAddress]::Parse('127.0.0.1'), 18080)
$listener.Start()
try {
    Invoke-Case 'занятый порт 18080' @('inet', 'stream', '127.0.0.1', '18080', '--bind') 4 'bind'
} finally {
    $listener.Stop()
}

Write-Host ''
if ($failed -eq 0) {
    Write-Host ("Все проверки пройдены: {0}/{0}" -f $total) -ForegroundColor Green
    exit 0
}
Write-Host ("Провалено {0} из {1}" -f $failed, $total) -ForegroundColor Red
exit 1
