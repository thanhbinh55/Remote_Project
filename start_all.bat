@echo off
REM Lấy đường dẫn thư mục chứa file .bat
set BASE_DIR=%~dp0

echo ===============================
echo PROJECT DIR: %BASE_DIR%
echo ===============================

REM ===== REGISTRY SERVER =====
echo START REGISTRY SERVER
start cmd /k "cd /d %BASE_DIR%registry_server && node server.js"

REM ===== WEB CLIENT =====
echo START WEB CLIENT
start cmd /k "cd /d %BASE_DIR%web-client && ng serve --open"

echo ===============================
echo ALL SERVICES STARTED
pause
