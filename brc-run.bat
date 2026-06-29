@echo off
setlocal enabledelayedexpansion

set PROJECT_DIR=%~dp0
set BUILD_DIR=%PROJECT_DIR%build
set BRICK=%BUILD_DIR%\brick.exe
set RUNTIME_DIR=%PROJECT_DIR%runtime

set BRC_FILE=%1
if "%BRC_FILE%"=="" (
    echo Uso: %~nx0 ^<arquivo.brc^>
    exit /b 1
)

for %%F in ("%BRC_FILE%") do set BASENAME=%%~nF
set C_FILE=%BUILD_DIR%\%BASENAME%.c
set BIN_FILE=%BUILD_DIR%\%BASENAME%.exe

echo --- %BASENAME% ---

"%BRICK%" "%BRC_FILE%" -o "%C_FILE%"
if %errorlevel% neq 0 exit /b %errorlevel%

gcc -O3 -I"%RUNTIME_DIR%" "%C_FILE%" ^
    "%RUNTIME_DIR%\block_memory.c" ^
    "%RUNTIME_DIR%\hot_reload.c" ^
    "%RUNTIME_DIR%\io.c" ^
    "%RUNTIME_DIR%\pool_allocator.c" ^
    -o "%BIN_FILE%"

"%BIN_FILE%"
