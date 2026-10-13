@echo off
setlocal
cd /d "%~dp0"
where cmake >nul 2>nul
if errorlevel 1 (
  echo Instala CMake y Visual Studio Build Tools con Desarrollo de escritorio con C++.
  echo Ya tienes el ejecutable listo en AeroRAM.exe; compilar es opcional.
  exit /b 1
)
cmake -S . -B build -A x64
if errorlevel 1 exit /b 1
cmake --build build --config Release
if errorlevel 1 exit /b 1
copy /Y "build\Release\AeroRAM.exe" "AeroRAM.exe"
echo Compilacion lista.
endlocal
