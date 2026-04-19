@echo off
chcp 65001 >nul
setlocal EnableDelayedExpansion

echo ========================================
echo WordGame AI Build Script
echo ========================================
echo.

REM 设置 curl 路径（自动搜索或手动指定）
if not defined CURL_PATH (
    echo [INFO] CURL_PATH not set, searching for curl...
    
    REM 首先检查常见位置
    if exist "..\curl-8.19.0_7-win64-mingw" (
        set "CURL_PATH=..\curl-8.19.0_7-win64-mingw"
        echo [FOUND] curl at: !CURL_PATH!
    ) else if exist "C:\curl" (
        set "CURL_PATH=C:\curl"
        echo [FOUND] curl at: !CURL_PATH!
    ) else (
        REM 在 PATH 中搜索
        for %%p in (curl.exe) do (
            set "CURL_BIN=%%~$PATH:p"
            if defined CURL_BIN (
                for %%d in ("!CURL_BIN!\..") do set "CURL_PATH=%%~fd"
                echo [FOUND] curl in PATH at: !CURL_PATH!
                goto :curl_found
            )
        )
        echo [ERROR] curl not found! Please install curl or set CURL_PATH manually.
        echo.
        echo Example: set CURL_PATH=C:\path\to\curl
        exit /b 1
    )
) else (
    echo [INFO] Using CURL_PATH: %CURL_PATH%
)

:curl_found

REM 设置 cjson 路径（自动搜索或手动指定）
if not defined CJSON_PATH (
    echo [INFO] CJSON_PATH not set, searching for cjson...
    
    REM 首先检查常见位置
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

REM 设置编译器标志
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

REM 运行 make
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
