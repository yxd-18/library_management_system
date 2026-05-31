param(
    [switch]$DryRun,
    [switch]$SkipBackup,
    [string]$TargetDatabase
)

$ErrorActionPreference = 'Stop'

$projectRoot = Split-Path -Parent $PSScriptRoot
$configPath = Join-Path $projectRoot 'db_config.txt'
$sqlPath = Join-Path $projectRoot 'library_management_system_postgresql.sql'
$backupDir = Join-Path $projectRoot 'db_backups'
$psqlPath = 'C:\Program Files\PostgreSQL\10\bin\psql.exe'
$pgDumpPath = 'C:\Program Files\PostgreSQL\10\bin\pg_dump.exe'
$createdbPath = 'C:\Program Files\PostgreSQL\10\bin\createdb.exe'

function Read-DbConfig {
    param([string]$Path)

    if (-not (Test-Path $Path)) {
        throw "Missing db config file: $Path"
    }

    $config = @{
        host = 'localhost'
        port = '5432'
        dbname = 'library_management_system'
        user = 'postgres'
        password = ''
    }

    Get-Content $Path | ForEach-Object {
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

    return $config
}

function Invoke-External {
    param(
        [string]$FilePath,
        [string[]]$Arguments
    )

    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed: $FilePath $($Arguments -join ' ')"
    }
}

function Invoke-ScalarQuery {
    param(
        [hashtable]$Config,
        [string]$Database,
        [string]$Sql
    )

    $result = & $psqlPath `
        -h $Config.host `
        -p $Config.port `
        -U $Config.user `
        -d $Database `
        -tAc $Sql

    if ($LASTEXITCODE -ne 0) {
        throw "Query failed on database $Database"
    }

    if ($null -eq $result) {
        return ''
    }

    return $result.Trim()
}

if (-not (Test-Path $sqlPath)) {
    throw "Missing SQL seed file: $sqlPath"
}

foreach ($tool in @($psqlPath, $pgDumpPath, $createdbPath)) {
    if (-not (Test-Path $tool)) {
        throw "Missing PostgreSQL tool: $tool"
    }
}

$config = Read-DbConfig -Path $configPath
if ($TargetDatabase) {
    $config.dbname = $TargetDatabase
}

$env:PGPASSWORD = $config.password

$dbExistsSql = "SELECT 1 FROM pg_database WHERE datname='$($config.dbname)';"
$dbExists = Invoke-ScalarQuery -Config $config -Database 'postgres' -Sql $dbExistsSql
$timestamp = Get-Date -Format 'yyyyMMdd_HHmmss'
$backupPath = Join-Path $backupDir ("{0}_before_full_seed_{1}.sql" -f $config.dbname, $timestamp)

Write-Host '=== Full test data import plan ==='
Write-Host ("Target database: {0}" -f $config.dbname)
Write-Host ("SQL file: {0}" -f $sqlPath)
Write-Host ("Database exists: {0}" -f ([bool]($dbExists -eq '1')))
Write-Host ("Backup enabled: {0}" -f (-not $SkipBackup))
Write-Host ("Dry run: {0}" -f $DryRun)

if ($DryRun) {
    if ($dbExists -eq '1' -and -not $SkipBackup) {
        Write-Host ("Would back up current database to: {0}" -f $backupPath)
    }
    elseif ($dbExists -ne '1') {
        Write-Host 'Would create target database before import.'
    }

    Write-Host 'Would then import library_management_system_postgresql.sql into the target database.'
    Write-Host 'Dry run finished. No changes were made.'
    exit 0
}

if (-not (Test-Path $backupDir)) {
    New-Item -ItemType Directory -Path $backupDir | Out-Null
}

if ($dbExists -eq '1' -and -not $SkipBackup) {
    Write-Host ("Backing up current database to: {0}" -f $backupPath)
    Invoke-External -FilePath $pgDumpPath -Arguments @(
        '-h', $config.host,
        '-p', $config.port,
        '-U', $config.user,
        '-d', $config.dbname,
        '-f', $backupPath
    )
}
elseif ($dbExists -ne '1') {
    Write-Host ("Creating database: {0}" -f $config.dbname)
    Invoke-External -FilePath $createdbPath -Arguments @(
        '-h', $config.host,
        '-p', $config.port,
        '-U', $config.user,
        $config.dbname
    )
}

Write-Host ("Importing full seed data into: {0}" -f $config.dbname)
Invoke-External -FilePath $psqlPath -Arguments @(
    '-h', $config.host,
    '-p', $config.port,
    '-U', $config.user,
    '-d', $config.dbname,
    '-f', $sqlPath
)

$verifyQueries = [ordered]@{
    roles = "SELECT COUNT(*) FROM roles;"
    users = "SELECT COUNT(*) FROM users;"
    book_categories = "SELECT COUNT(*) FROM book_categories;"
    books = "SELECT COUNT(*) FROM books;"
    borrow_records = "SELECT COUNT(*) FROM borrow_records;"
    reservation_records = "SELECT COUNT(*) FROM reservation_records;"
    operation_logs = "SELECT COUNT(*) FROM operation_logs;"
    user_settings = "SELECT COUNT(*) FROM user_settings;"
    backup_records = "SELECT COUNT(*) FROM backup_records;"
}

Write-Host ''
Write-Host '=== Import verification ==='
foreach ($entry in $verifyQueries.GetEnumerator()) {
    $count = Invoke-ScalarQuery -Config $config -Database $config.dbname -Sql $entry.Value
    Write-Host ("{0}: {1}" -f $entry.Key, $count)
}

Write-Host ''
Write-Host 'Import finished successfully.'
Write-Host 'Default accounts from seed file:'
Write-Host 'admin01 / admin123'
Write-Host 'admin02 / admin123'
Write-Host 'reader01 / reader123'
Write-Host 'reader02 / reader123'
Write-Host 'reader03 / reader123'
Write-Host 'reader04 / reader123'

if (-not $SkipBackup -and (Test-Path $backupPath)) {
    Write-Host ("Backup file: {0}" -f $backupPath)
}
