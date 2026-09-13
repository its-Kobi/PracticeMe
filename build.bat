@echo off
setlocal
echo === PracticeMe build.bat ===
where cmake >nul 2>&1
if errorlevel 1 ( echo [ERROR] cmake not found & exit /b 1)
where git >nul 2>&1
if errorlevel 1 ( echo [WARN] git not found -- Geode fetch may fail)
if not exist build mkdir build
cmake -B build -G Ninja 2>nul
if errorlevel 1 (
  echo Ninja not found, trying default generator...
  cmake -B build
)
if errorlevel 1 ( echo [ERROR] cmake configure failed & exit /b 1)
cmake --build build --config Release
if errorlevel 1 ( echo [ERROR] build failed & exit /b 1)
set GEODE_FILE=
for /R build %%f in (*.geode) do set GEODE_FILE=%%f
if not defined GEODE_FILE ( echo [ERROR] .geode not found & exit /b 1)
if not exist "%USERPROFILE%\Desktop\PracticeMe-Release" mkdir "%USERPROFILE%\Desktop\PracticeMe-Release"
copy /Y "%GEODE_FILE%" "%USERPROFILE%\Desktop\PracticeMe-Release\PracticeMe.geode"
echo [OK] Copied to Desktop\PracticeMe-Release\PracticeMe.geode
