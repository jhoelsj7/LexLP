@echo off
REM Compila LexLP con MinGW-w64 (g++).
REM -static: el .exe queda autocontenido y corre sin DLLs adicionales.

set GXX=g++
where g++ >nul 2>nul || set GXX=C:\mingw64\bin\g++.exe

"%GXX%" -std=c++17 -Wall -Wextra -static -o LexLP.exe src\main.cpp src\Lexer.cpp src\Token.cpp
if %errorlevel% neq 0 (
    echo.
    echo Fallo la compilacion.
    exit /b %errorlevel%
)
echo Compilado correctamente: LexLP.exe
