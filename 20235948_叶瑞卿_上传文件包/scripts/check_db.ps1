$ErrorActionPreference = 'Stop'

$projectRoot = Split-Path -Parent $PSScriptRoot
$configPath = Join-Path $projectRoot 'db_config.txt'
$psqlPath = 'C:\Program Files\PostgreSQL\10\bin\psql.exe'

if (-not (Test-Path $configPath)) {
    Write-Error "Missing db config file: $configPath"
}

if (-not (Test-Path $psqlPath)) {
    Write-Error "Missing psql.exe: $psqlPath"
}

$config = @{
    host = 'localhost'
    port = '5432'
    dbname = 'library_management_system'
    user = 'postgres'
    password = ''
}

Get-Content $configPath | ForEach-Object {
    $line = $_.Trim()
    if (-not $line -or $line.StartsWith('#')) {
        return
    }

    $parts = $line -split '=', 2
    if ($parts.Count -ne 2) {
        return
    }

    $key = $parts[0].Trim()
    $value = $parts[1].Trim()
    if ($config.ContainsKey($key)) {
        $config[$key] = $value
    }
}

$serviceInfo = Get-Service | Where-Object {
    $_.Name -like 'postgres*' -or $_.DisplayName -like '*PostgreSQL*'
} | Select-Object -First 1

Write-Host '=== PostgreSQL service ==='
if ($serviceInfo) {
    Write-Host ("Service: {0} | Status: {1}" -f $serviceInfo.Name, $serviceInfo.Status)
}
else {
    Write-Warning 'PostgreSQL service was not found.'
}

$portTest = Test-NetConnection -ComputerName $config.host -Port ([int]$config.port) -WarningAction SilentlyContinue
Write-Host ''
Write-Host '=== Port check ==='
Write-Host ("{0}:{1} -> {2}" -f $config.host, $config.port, $portTest.TcpTestSucceeded)

$env:PGPASSWORD = $config.password

Write-Host ''
Write-Host '=== Database connection ==='
$dbExistsQuery = "SELECT 1 FROM pg_database WHERE datname='$($config.dbname)';"
$dbExists = & $psqlPath -h $config.host -p $config.port -U $config.user -d postgres -tAc $dbExistsQuery
if ($LASTEXITCODE -ne 0) {
    Write-Error 'Cannot connect to PostgreSQL with current config.'
}

if (-not $dbExists -or $dbExists.Trim() -ne '1') {
    Write-Warning ("Database not found: {0}" -f $config.dbname)
    exit 1
}

Write-Host ("Database exists and login works: {0}" -f $config.dbname)

$tableCount = & $psqlPath -h $config.host -p $config.port -U $config.user -d $config.dbname -tAc "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema='public';"
$userCount = & $psqlPath -h $config.host -p $config.port -U $config.user -d $config.dbname -tAc "SELECT COUNT(*) FROM users;"
$bookCount = & $psqlPath -h $config.host -p $config.port -U $config.user -d $config.dbname -tAc "SELECT COUNT(*) FROM books;"
$borrowCount = & $psqlPath -h $config.host -p $config.port -U $config.user -d $config.dbname -tAc "SELECT COUNT(*) FROM borrow_records;"

Write-Host ''
Write-Host '=== Data summary ==='
Write-Host ("Table count: {0}" -f $tableCount.Trim())
Write-Host ("User count: {0}" -f $userCount.Trim())
Write-Host ("Book count: {0}" -f $bookCount.Trim())
Write-Host ("Borrow record count: {0}" -f $borrowCount.Trim())

Write-Host ''
Write-Host 'Database check finished.'
