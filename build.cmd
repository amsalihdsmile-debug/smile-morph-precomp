@echo off
setlocal
if not defined AE_SDK_EXAMPLES (
  echo Set AE_SDK_EXAMPLES to the Adobe After Effects SDK Examples directory.
  exit /b 2
)
if not exist "%AE_SDK_EXAMPLES%\Headers\AE_Effect.h" (
  echo AE_SDK_EXAMPLES does not point to the SDK Examples directory.
  exit /b 2
)
where cl >nul 2>nul
if errorlevel 1 (
  echo Run this script from an x64 Native Tools command prompt for Visual Studio.
  exit /b 2
)
where rc >nul 2>nul
if errorlevel 1 (
  echo Windows Resource Compiler rc.exe was not found on PATH.
  exit /b 2
)
where python >nul 2>nul
if errorlevel 1 (
  echo Python 3 was not found on PATH.
  exit /b 2
)
pushd "%~dp0"
if not exist build mkdir build
python src\make_resource.py
if errorlevel 1 goto :fail
rc /nologo /I build /fo build\SmileMorphPrecomp.res build\SmileMorphPrecomp.rc
if errorlevel 1 goto :fail
cl /nologo /std:c++17 /EHsc /O2 /MT /LD /D MSWindows /D WIN32 /D _WINDOWS /I "%AE_SDK_EXAMPLES%\Headers" /I "%AE_SDK_EXAMPLES%\Headers\SP" /I "%AE_SDK_EXAMPLES%\Util" /I "%AE_SDK_EXAMPLES%\Headers\Win" src\SmileMorph.cpp /Fobuild\SmileMorph.obj build\SmileMorphPrecomp.res /link /OUT:SmileMorphPrecomp.aex /IMPLIB:build\SmileMorph.lib
if errorlevel 1 goto :fail
echo Built unsigned SmileMorphPrecomp.aex. Validate it in After Effects before distribution.
popd
exit /b 0
:fail
popd
exit /b 1
