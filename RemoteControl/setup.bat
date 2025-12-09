@echo off
:: CHUYEN VE THU MUC CHUA FILE BAT (QUAN TRONG)
cd /d "%~dp0"
:: =========================================================
:: SCRIPT CAI DAT REMOTE SERVER (RUN AS ADMINISTRATOR)
:: =========================================================
title CAI DAT REMOTE SERVER
color 0A

:: 1. KIEM TRA QUYEN ADMIN
:: Lenh nay se loi neu khong co quyen Admin
net session >nul 2>&1
if %errorLevel% == 0 (
    echo [OK] Da co quyen Administrator.
) else (
    echo =========================================================
    echo [LOI] BAN CHUA CHAY FILE NAY BANG QUYEN ADMIN!
    echo Vui long chuot phai vao file -> Chon 'Run as Administrator'
    echo =========================================================
    pause
    exit
)

echo.
echo DANG CAU HINH HE THONG... VUI LONG DOI...
echo ------------------------------------------

:: 2. CAU HINH WINDOWS DEFENDER (Them thu muc hien tai vao Exclusions)
:: De tranh bi Defender xoa file server.exe
echo [1/3] Them thu muc nay vao danh sach tin cay...
powershell -inputformat none -outputformat none -NonInteractive -Command "Add-MpPreference -ExclusionPath '%cd%'"

:: 3. MO PORT FIREWALL (Cong 9010)
:: Cho phep ket noi tu Client vao Server
echo [2/3] Mo cong tuong lua (Port 9010)...
:: Xoa rule cu neu co de tranh trung lap
netsh advfirewall firewall delete rule name="RemoteServerAllow" >nul
:: Them rule moi cho ca TCP
netsh advfirewall firewall add rule name="RemoteServerAllow" dir=in action=allow protocol=TCP localport=9010

:: 4. CHAY SERVER
echo [3/3] Khoi dong Server...
echo.
echo =========================================================
echo CAI DAT HOAN TAT! SERVER DANG CHAY...
echo DIA CHI IP CUA MAY NAY LA:
echo =========================================================
:: Hien thi IP de ban biet duong ket noi tu Client
ipconfig | findstr /i "IPv4"
echo =========================================================
echo.

:: Khoi dong server.exe (Dung start "" de khong bi treo cua so CMD nay)
start "" "server.exe"

:: Dong cua so setup sau 10 giay (hoac bo dong nay neu muon giu lai de xem IP)
timeout /t 15
exit