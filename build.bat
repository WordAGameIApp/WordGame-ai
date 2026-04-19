@echo off
setlocal EnableDelayedExpansion

echo ========================================
echo WordGame AI Build Script
echo ========================================
echo.

REM Set curl path (auto-detect or manual)
if not defined CURL_PATH (
    echo [INFO] CURL_PATH not set, searching for curl...
    
    REM Check common locations first
    if exist "..\curl-8.19.0_7-win64-mingw" (
        set "CURL_PATH=..\curl-8.19.0_7-win64-mingw"
        echo [FOUND] curl at: !CURL_PATH!
    ) else if exist "C:\curl" (
        set "CURL_PATH=C:\curl"
        echo [FOUND] curl at: !CURL_PATH!
    ) else (
        echo [ERROR] curl not found! Please install curl or set CURL_PATH manually.
        echo.
        echo Example: set CURL_PATH=C:\path\to\curl
        exit /b 1
    )
) else (
    echo [INFO] Using CURL_PATH: %CURL_PATH%
)

REM Set cjson path (auto-detect or manual)
if not defined CJSON_PATH (
    echo [INFO] CJSON_PATH not set, searching for cjson...
    
    REM Check common locations first
    if exist "..\cjson" (
        set "CJSON_PATH=..\cjson"
        echo [FOUND] cjson at: !CJSON_PATH!
    ) else if exist "..\cJSON" (
        set "CJSON_PATH=..\cJSON"
        echo [FOUND] cjson at: !CJSON_PATH!
    ) else if exist "C:\cjson" (
        set "CJSON_PATH=C:\cjson"
        echo [FOUND] cjson at: !CJSON_PATH!
    ) else (
        echo [ERROR] cjson not found! Please install cjson or set CJSON_PATH manually.
        echo.
        echo Example: set CJSON_PATH=C:\path\to\cjson
        exit /b 1
    )
) else (
    echo [INFO] Using CJSON_PATH: %CJSON_PATH%
)

echo.
echo ========================================
echo Setting up environment variables...
echo ========================================

REM Set compiler flags
set "CURL_CFLAGS=-I%CURL_PATH%\include"
set "CURL_LDFLAGS=-L%CURL_PATH%\lib -lcurl"
set "CJSON_CFLAGS=-I%CJSON_PATH%"
set "CJSON_LDFLAGS=-L%CJSON_PATH% -lcjson"

echo CURL_CFLAGS=%CURL_CFLAGS%
echo CURL_LDFLAGS=%CURL_LDFLAGS%
echo CJSON_CFLAGS=%CJSON_CFLAGS%
echo CJSON_LDFLAGS=%CJSON_LDFLAGS%

echo.
echo ========================================
echo Building WordGame AI...
echo ========================================
echo.

REM Run make
make %*

if %ERRORLEVEL% neq 0 (
    echo.
    echo [ERROR] Build failed!
    exit /b %ERRORLEVEL%
)

echo.
echo ========================================
echo Build completed successfully!
echo ========================================
echo.
echo Output: build\api_client.exe

endlocal
