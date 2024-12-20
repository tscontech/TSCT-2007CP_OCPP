if "%VERBOSE%"=="1" (
    @echo on
) else (
    @echo off
)

call common.cmd
set CFG_DEVELOP=0
set CFG_VER_INTERNATIONAL=0
set /A processMax=%NUMBER_OF_PROCESSORS%*2
set CL=/MP%processMax%

if "%TARGET%"=="" set TARGET=%CFG_PROJECT%
if "%AUTOBUILD%"=="" set AUTOBUILD=0

rem ==========================================================================
rem set AUTOBACKUPCONFIG=1 if you want to backup the .config to the porject dir.
rem ==========================================================================
set AUTOBACKUPCONFIG=0

if "%CFG_WIN32_NMAKE%"=="1"  (
    set BUILD_CMD=post_build.cmd
    set RUN_CMD=%CMAKE_SOURCE_DIR%\build\%CFG_PLATFORM%\%CFG_PROJECT%\project\%TARGET%\exec.cmd

    mkdir %CFG_PROJECT%

    if not exist "%CMAKE_SOURCE_DIR%/project/%CFG_PROJECT%/config.cmake" (
        call :open_qconf
        if errorlevel 1 exit /b
    ) else (
        call post_build.cmd
    )
) else (
    if not exist %CFG_PROJECT% mkdir %CFG_PROJECT%
    if not exist "%CMAKE_SOURCE_DIR%/project/%CFG_PROJECT%/config.cmake" (
        title Configuration
        chcp 437 > nul

        call :open_qconf
        if errorlevel 1 exit /b
    )
    cd %CFG_PROJECT%
    if "%VS16%"=="1" (
        cmake.exe -G"Visual Studio 16 2019" -A Win32 -DCMAKE_SYSTEM_VERSION=%VCVARS2% "%CMAKE_SOURCE_DIR%"
    ) else if "%VS12%"=="1" (
        cmake.exe -G"Visual Studio 12 2013" -T v120_xp "%CMAKE_SOURCE_DIR%"
    ) else if "%VS12%" == "" (
        cmake.exe -G"Visual Studio 9 2008" "%CMAKE_SOURCE_DIR%"
    ) else if "%VS14%"=="1" (
        cmake.exe -G"Visual Studio 14 2015" -T v140_xp "%CMAKE_SOURCE_DIR%"
    ) else if "%VS15%"=="1" (
        cmake.exe -G"Visual Studio 15 2017" "%CMAKE_SOURCE_DIR%"
    )

    if errorlevel 1 exit /b
)

if "%AUTOBUILD%"=="1" (
    MSBuild "%CMAKE_SOURCE_DIR%\build\%CFG_PLATFORM%\%CFG_PROJECT%\%CFG_PROJECT%.sln" -maxcpucount
)
goto :eof

:open_qconf
rem ==========================================================================
rem open qconf. The qconf must be run from build/xxx/ directory, otherwise
rem some relative path in the kconfig files needs to be fixed.
rem ==========================================================================
if "%AUTOBACKUPCONFIG%"=="1" (
    if exist "%CMAKE_SOURCE_DIR%/project/%CFG_PROJECT%/.config.%CFG_PLATFORM%" (
        cmake.exe -E copy "%CMAKE_SOURCE_DIR%/project/%CFG_PROJECT%/.config.%CFG_PLATFORM%" %CFG_PROJECT%/.config
    )
)
if "%AUTOBUILD%"=="1" (
    mconf --autowrite --prefix "CFG_" --cmakefile %CFG_PROJECT%/config.cmake --cfgfile %CFG_PROJECT%/.config "%CMAKE_SOURCE_DIR%/project/%CFG_PROJECT%/Kconfig.win32"
) else (
    qconf --prefix "CFG_" --cmakefile %CFG_PROJECT%/config.cmake --cfgfile %CFG_PROJECT%/.config "%CMAKE_SOURCE_DIR%/project/%CFG_PROJECT%/Kconfig.win32"
)
if "%AUTOBACKUPCONFIG%"=="1" (
    cmake.exe -E copy %CFG_PROJECT%/.config "%CMAKE_SOURCE_DIR%/project/%CFG_PROJECT%/.config.%CFG_PLATFORM%"
)
goto :eof