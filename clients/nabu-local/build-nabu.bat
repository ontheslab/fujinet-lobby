@echo off
setlocal

rem Toolchain setup
set Z88DK_DIR=c:\z88dk\
set ZCCCFG=%Z88DK_DIR%lib\config\
set PATH=%Z88DK_DIR%bin;%PATH%

rem Deploy paths
set STORE_DIR=D:\NIA\NABU Internet Adapter\Store
set DRIVE_DIR=D:\NIA\NABU Internet Adapter\Store\D\0
set SCRIPT_DIR=%~dp0
set OUTPUT_DIR=%SCRIPT_DIR%r2r\nabu_cpm
set SRC_DIR=%SCRIPT_DIR%..\src\nabu

if /I "%~1"=="clean" goto :clean

echo.
echo ****************************************************************************
echo  Building FujiNet Lobby NABU Local Client
echo ****************************************************************************

if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

pushd "%SCRIPT_DIR%"

zcc +cpm -subtype=nabu -vn --list -m -create-app -compiler=sdcc -O3 --opt-code-speed ^
 -I..\include ^
 -I..\include\nabu ^
 -I..\src\nabu ^
 ..\src\nabu\main.c ^
 -o r2r\nabu_cpm\NLOBBY

if errorlevel 1 goto :fail

if not exist "%STORE_DIR%" mkdir "%STORE_DIR%"
if not exist "%DRIVE_DIR%" mkdir "%DRIVE_DIR%"

copy /Y "r2r\nabu_cpm\NLOBBY.com" "%DRIVE_DIR%\"
copy /Y "NLOBBY.CFG" "%STORE_DIR%\"

call :clean_temp

echo.
echo ****************************************************************************
echo  Done.
echo    NLOBBY.com copied to %DRIVE_DIR%
echo    NLOBBY.CFG copied to %STORE_DIR%
echo ****************************************************************************

popd
exit /b 0

:fail
echo.
echo Build failed.
popd
exit /b 1

:clean
echo.
echo ****************************************************************************
echo  Cleaning FujiNet Lobby NABU Local Client
echo ****************************************************************************
call :clean_all
echo.
echo Done.
exit /b 0

:clean_temp
if exist "%OUTPUT_DIR%\NLOBBY" del /Q "%OUTPUT_DIR%\NLOBBY"
if exist "%OUTPUT_DIR%\NLOBBY.img" del /Q "%OUTPUT_DIR%\NLOBBY.img"
if exist "%OUTPUT_DIR%\NLOBBY.lis" del /Q "%OUTPUT_DIR%\NLOBBY.lis"
if exist "%OUTPUT_DIR%\NLOBBY.map" del /Q "%OUTPUT_DIR%\NLOBBY.map"
if exist "%SRC_DIR%\main.c.lis" del /Q "%SRC_DIR%\main.c.lis"
if exist "%SRC_DIR%\config.c.lis" del /Q "%SRC_DIR%\config.c.lis"
if exist "%SRC_DIR%\net.c.lis" del /Q "%SRC_DIR%\net.c.lis"
if exist "%SRC_DIR%\ui.c.lis" del /Q "%SRC_DIR%\ui.c.lis"
exit /b 0

:clean_all
if exist "%OUTPUT_DIR%\NLOBBY" del /Q "%OUTPUT_DIR%\NLOBBY"
if exist "%OUTPUT_DIR%\NLOBBY.com" del /Q "%OUTPUT_DIR%\NLOBBY.com"
if exist "%OUTPUT_DIR%\NLOBBY.img" del /Q "%OUTPUT_DIR%\NLOBBY.img"
if exist "%OUTPUT_DIR%\NLOBBY.lis" del /Q "%OUTPUT_DIR%\NLOBBY.lis"
if exist "%OUTPUT_DIR%\NLOBBY.map" del /Q "%OUTPUT_DIR%\NLOBBY.map"
if exist "%SRC_DIR%\main.c.lis" del /Q "%SRC_DIR%\main.c.lis"
if exist "%SRC_DIR%\config.c.lis" del /Q "%SRC_DIR%\config.c.lis"
if exist "%SRC_DIR%\net.c.lis" del /Q "%SRC_DIR%\net.c.lis"
if exist "%SRC_DIR%\ui.c.lis" del /Q "%SRC_DIR%\ui.c.lis"
exit /b 0
