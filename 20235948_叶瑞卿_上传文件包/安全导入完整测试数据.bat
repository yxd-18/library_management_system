@echo off
chcp 65001>nul
cd /d "%~dp0"
powershell -ExecutionPolicy Bypass -File ".\scripts\import_full_test_data.ps1"
