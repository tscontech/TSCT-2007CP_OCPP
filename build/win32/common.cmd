@if defined COMMON_DEFINED goto end
set COMMON_DEFINED=1

call :get_cfg_platform "%cd%"

rem ==========================================================================
rem set CMAKE_SOURCE_DIR=build\win32\..\..\
rem ==========================================================================
pushd ..\..
for /f "delims=" %%a in ('cd') do set CMAKE_SOURCE_DIR=%%a
popd

set CFG_BUILDPLATFORM=%CFG_PLATFORM%
set CMAKE_ROOT=%CMAKE_SOURCE_DIR%\tool
set PATH=%CMAKE_ROOT%\bin;%PATH%

if "%AUTOBUILD%"=="1" (
    if exist "C:/Program Files (x86)/Microsoft Visual Studio 12.0/VC" (
        set CFG_VC_INC_PATH=C:/Program Files ^(x86^)/Microsoft Visual Studio 12.0/VC/include
        set CFG_VC_PATH=C:/Program Files ^(x86^)/Microsoft Visual Studio 12.0/VC
        set CFG_WINSDK_INC_PATH=C:/Program Files ^(x86^)/Microsoft Visual Studio 12.0/VC/include
        set VS=3
        set VS12=1
    ) else if exist "C:/Program Files/Microsoft Visual Studio 12.0/VC" (
        set CFG_VC_INC_PATH=C:/Program Files/Microsoft Visual Studio 12.0/VC/include
        set CFG_VC_PATH=C:/Program Files/Microsoft Visual Studio 12.0/VC
        set CFG_WINSDK_INC_PATH=C:/Program Files/Microsoft Visual Studio 12.0/VC/include
        set VS=2
        set VS12=1
    ) else if exist "C:/Program Files (x86)/Microsoft Visual Studio 9.0/VC" (
        set CFG_VC_INC_PATH=C:/Program Files ^(x86^)/Microsoft Visual Studio 9.0/VC/include
        set CFG_VC_PATH=C:/Program Files ^(x86^)/Microsoft Visual Studio 9.0/VC
        set CFG_WINSDK_INC_PATH=C:/Program Files ^(x86^)/Microsoft Visual Studio 9.0/VC/include
        set VS=0
        set VS12=
    ) else if exist "C:/Program Files/Microsoft Visual Studio 9.0/VC" (
        set CFG_VC_INC_PATH=C:/Program Files/Microsoft Visual Studio 9.0/VC/include
        set CFG_VC_PATH=C:/Program Files/Microsoft Visual Studio 9.0/VC
        set CFG_WINSDK_INC_PATH=C:/Program Files/Microsoft Visual Studio 9.0/VC/include
        set VS=1
        set VS12=
    ) else if exist "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC" (
        set CFG_VC_INC_PATH=C:/Program Files ^(x86^)/Microsoft Visual Studio 14.0/VC/include
        set CFG_VC_PATH=C:/Program Files ^(x86^)/Microsoft Visual Studio 14.0/VC
        set CFG_WINSDK_INC_PATH=C:/Program Files ^(x86^)/Microsoft Visual Studio 14.0/VC/include
        set VS=5
        set VS14=1
    ) else if exist "C:/Program Files/Microsoft Visual Studio 14.0/VC" (
        set CFG_VC_INC_PATH=C:/Program Files/Microsoft Visual Studio 14.0/VC/include
        set CFG_VC_PATH=C:/Program Files/Microsoft Visual Studio 14.0/VC
        set CFG_WINSDK_INC_PATH=C:/Program Files/Microsoft Visual Studio 14.0/VC/include
        set VS=4
        set VS14=1
    ) else if exist "C:/Program Files (x86)/Microsoft Visual Studio/2017/Community/VC" (
        set CFG_VC_INC_PATH=C:/Program Files ^(x86^)/Microsoft Visual Studio/2017/Community/VC/Tools/MSVC/14.12.25827/include
        set CFG_VC_PATH=C:/Program Files ^(x86^)/Microsoft Visual Studio/2017/Community/VC/Auxiliary/Build
        set CFG_WINSDK_INC_PATH=C:/Program Files ^(x86^)/Windows Kits/10/Include/10.0.10240.0/ucrt
        set VS=6
        set VS15=1
    ) else if exist "C:/Program Files (x86)/Microsoft Visual Studio/2017/Professional/VC" (
        set CFG_VC_INC_PATH=C:/Program Files ^(x86^)/Microsoft Visual Studio/2017/Professional/VC/Tools/MSVC/14.12.25827/include
        set CFG_VC_PATH=C:/Program Files ^(x86^)/Microsoft Visual Studio/2017/Professional/VC/Auxiliary/Build
        set CFG_WINSDK_INC_PATH=C:/Program Files ^(x86^)/Windows Kits/10/Include/10.0.10240.0/ucrt
        set VS=6
        set VS15=1
    ) else if exist "C:/Program Files (x86)/Microsoft Visual Studio/2017/Enterprise/VC" (
        set CFG_VC_INC_PATH=C:/Program Files ^(x86^)/Microsoft Visual Studio/2017/Enterprise/VC/Tools/MSVC/14.12.25827/include
        set CFG_VC_PATH=C:/Program Files ^(x86^)/Microsoft Visual Studio/2017/Enterprise/VC/Auxiliary/Build
        set CFG_WINSDK_INC_PATH=C:/Program Files ^(x86^)/Windows Kits/10/Include/10.0.10240.0/ucrt
        set VS=6
        set VS15=1
    )

) else (
    if exist "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC" (
        set CFG_VC_INC_PATH=C:/Program Files ^(x86^)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.23.28105/include
        set CFG_VC_PATH=C:/Program Files ^(x86^)/Microsoft Visual Studio/2019/Community/VC/Auxiliary/Build
        set CFG_WINSDK_INC_PATH=C:/Program Files ^(x86^)/Windows Kits/10/Include/10.0.10240.0/ucrt
        set VS=7
        set VS16=1
        set VCVARS1=x86
        set VCVARS2=10.0.10240.0
    ) else if exist "C:/Program Files (x86)/Microsoft Visual Studio/2019/Professional/VC" (
        set CFG_VC_INC_PATH=C:/Program Files ^(x86^)/Microsoft Visual Studio/2019/Professional/VC/Tools/MSVC/14.23.28105/include
        set CFG_VC_PATH=C:/Program Files ^(x86^)/Microsoft Visual Studio/2019/Professional/VC/Auxiliary/Build
        set CFG_WINSDK_INC_PATH=C:/Program Files ^(x86^)/Windows Kits/10/Include/10.0.10240.0/ucrt
        set VS=7
        set VS16=1
        set VCVARS1=x86
        set VCVARS2=10.0.10240.0
    ) else if exist "C:/Program Files (x86)/Microsoft Visual Studio/2019/Enterprise/VC" (
        set CFG_VC_INC_PATH=C:/Program Files ^(x86^)/Microsoft Visual Studio/2019/Enterprise/VC/Tools/MSVC/14.23.28105/include
        set CFG_VC_PATH=C:/Program Files ^(x86^)/Microsoft Visual Studio/2019/Enterprise/VC/Auxiliary/Build
        set CFG_WINSDK_INC_PATH=C:/Program Files ^(x86^)/Windows Kits/10/Include/10.0.10240.0/ucrt
        set VS=7
        set VS16=1
        set VCVARS1=x86
        set VCVARS2=10.0.10240.0
    ) else if exist "C:/Program Files (x86)/Microsoft Visual Studio 12.0/VC" (
        set CFG_VC_INC_PATH=C:/Program Files ^(x86^)/Microsoft Visual Studio 12.0/VC/include
        set CFG_VC_PATH=C:/Program Files ^(x86^)/Microsoft Visual Studio 12.0/VC
        set CFG_WINSDK_INC_PATH=C:/Program Files ^(x86^)/Microsoft Visual Studio 12.0/VC/include
        set VS=3
        set VS12=1
    ) else if exist "C:/Program Files/Microsoft Visual Studio 12.0/VC" (
        set CFG_VC_INC_PATH=C:/Program Files/Microsoft Visual Studio 12.0/VC/include
        set CFG_VC_PATH=C:/Program Files/Microsoft Visual Studio 12.0/VC
        set CFG_WINSDK_INC_PATH=C:/Program Files/Microsoft Visual Studio 12.0/VC/include
        set VS=2
        set VS12=1
    ) else if exist "C:/Program Files (x86)/Microsoft Visual Studio 9.0/VC" (
        set CFG_VC_INC_PATH=C:/Program Files ^(x86^)/Microsoft Visual Studio 9.0/VC/include
        set CFG_VC_PATH=C:/Program Files ^(x86^)/Microsoft Visual Studio 9.0/VC
        set CFG_WINSDK_INC_PATH=C:/Program Files ^(x86^)/Microsoft Visual Studio 9.0/VC/include
        set VS=0
        set VS12=
    ) else if exist "C:/Program Files/Microsoft Visual Studio 9.0/VC" (
        set CFG_VC_INC_PATH=C:/Program Files/Microsoft Visual Studio 9.0/VC/include
        set CFG_VC_PATH=C:/Program Files/Microsoft Visual Studio 9.0/VC
        set CFG_WINSDK_INC_PATH=C:/Program Files/Microsoft Visual Studio 9.0/VC/include
        set VS=1
        set VS12=
    ) else if exist "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC" (
        set CFG_VC_INC_PATH=C:/Program Files ^(x86^)/Microsoft Visual Studio 14.0/VC/include
        set CFG_VC_PATH=C:/Program Files ^(x86^)/Microsoft Visual Studio 14.0/VC
        set CFG_WINSDK_INC_PATH=C:/Program Files ^(x86^)/Microsoft SDKs/Windows/v7.1A/Include
        set VS=5
        set VS14=1
    ) else if exist "C:/Program Files/Microsoft Visual Studio 14.0/VC" (
        set CFG_VC_INC_PATH=C:/Program Files/Microsoft Visual Studio 14.0/VC/include
        set CFG_VC_PATH=C:/Program Files/Microsoft Visual Studio 14.0/VC
        set CFG_WINSDK_INC_PATH=C:/Program Files/Microsoft Visual Studio 14.0/VC/include
        set VS=4
        set VS14=1
    ) else  if exist "C:/Program Files (x86)/Microsoft Visual Studio/2017/WDExpress/VC" (
        set CFG_VC_INC_PATH=C:/Program Files ^(x86^)/Microsoft Visual Studio/2017/WDExpress/VC/Tools/MSVC/14.16.27023/include
        set CFG_VC_PATH=C:/Program Files ^(x86^)/Microsoft Visual Studio/2017/WDExpress/VC/Auxiliary/Build
        set CFG_WINSDK_INC_PATH=C:/Program Files ^(x86^)/Windows Kits/10/Include/10.0.10240.0/ucrt
        set VS=6
        set VS15=1
        set VCVARS1=x86
        set VCVARS2=10.0.10240.0
    ) else if exist "C:/Program Files (x86)/Microsoft Visual Studio/2017/Community/VC" (
        set CFG_VC_INC_PATH=C:/Program Files ^(x86^)/Microsoft Visual Studio/2017/Community/VC/Tools/MSVC/14.12.25827/include
        set CFG_VC_PATH=C:/Program Files ^(x86^)/Microsoft Visual Studio/2017/Community/VC/Auxiliary/Build
        set CFG_WINSDK_INC_PATH=C:/Program Files ^(x86^)/Windows Kits/10/Include/10.0.10240.0/ucrt
        set VS=6
        set VS15=1
    ) else if exist "C:/Program Files (x86)/Microsoft Visual Studio/2017/Professional/VC" (
        set CFG_VC_INC_PATH=C:/Program Files ^(x86^)/Microsoft Visual Studio/2017/Professional/VC/Tools/MSVC/14.12.25827/include
        set CFG_VC_PATH=C:/Program Files ^(x86^)/Microsoft Visual Studio/2017/Professional/VC/Auxiliary/Build
        set CFG_WINSDK_INC_PATH=C:/Program Files ^(x86^)/Windows Kits/10/Include/10.0.10240.0/ucrt
        set VS=6
        set VS15=1
    ) else if exist "C:/Program Files (x86)/Microsoft Visual Studio/2017/Enterprise/VC" (
        set CFG_VC_INC_PATH=C:/Program Files ^(x86^)/Microsoft Visual Studio/2017/Enterprise/VC/Tools/MSVC/14.12.25827/include
        set CFG_VC_PATH=C:/Program Files ^(x86^)/Microsoft Visual Studio/2017/Enterprise/VC/Auxiliary/Build
        set CFG_WINSDK_INC_PATH=C:/Program Files ^(x86^)/Windows Kits/10/Include/10.0.10240.0/ucrt
        set VS=6
        set VS15=1
    )
)

set VS=%VS%

call "%CFG_VC_PATH%\vcvarsall.bat" %VCVARS1% %VCVARS2%

:end
goto :eof

:get_cfg_platform
rem ==========================================================================
rem set CFG_PLATFORM=win32 from the current directory name build\[win32]
rem ==========================================================================
set CFG_PLATFORM=%~n1
goto :eof
