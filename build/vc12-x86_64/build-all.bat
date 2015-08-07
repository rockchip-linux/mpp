@echo off
if "%VS120COMNTOOLS%" == "" (
  msg "%username%" "Visual Studio 12 not detected"
  exit 1
)
if not exist rk_mpp.sln (
  call make-solutions.bat
)
if exist rk_mpp.sln (
  call "%VS120COMNTOOLS%\..\..\VC\vcvarsall.bat"
  rem MSBuild /property:Configuration="Release" rk_mpp.sln
  MSBuild /property:Configuration="Debug" rk_mpp.sln
  rem MSBuild /property:Configuration="RelWithDebInfo" rk_mpp.sln
)