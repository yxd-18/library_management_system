@echo off
setlocal
cd /d "%~dp0"

if not exist "db_config.txt" (
    if exist "db_config.example.txt" (
        copy /Y "db_config.example.txt" "db_config.txt" >nul
        echo 已自动生成 db_config.txt，请先按你的 PostgreSQL 环境修改其中的密码后再重新启动。
        echo.
        pause
        exit /b 1
    ) else (
        echo 缺少 db_config.example.txt，无法生成数据库配置文件。
        pause
        exit /b 1
    )
)

start "" "library_management_system_qt.exe"
