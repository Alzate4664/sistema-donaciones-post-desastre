@echo off
setlocal
where cl >nul 2>nul
if errorlevel 1 (
  if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    call "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
  ) else (
    echo Abra una consola Developer Command Prompt de Visual Studio con C++ y CMake.
    exit /b 1
  )
)
where cmake >nul 2>nul
if errorlevel 1 set "PATH=%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;%PATH%"
cmake -S "%~dp0." -B "%~dp0build" -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS_DEBUG="/Od /Ob0" -DCMAKE_CXX_FLAGS_RELWITHDEBINFO="/O2 /Ob1 /DNDEBUG"
if errorlevel 1 exit /b 1
cmake --build "%~dp0build"
if errorlevel 1 exit /b 1
ctest --test-dir "%~dp0build" --output-on-failure
