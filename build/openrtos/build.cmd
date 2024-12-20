@echo off
rem set VERBOSE=1

call common.cmd
chcp 437 > nul

if not exist %CFG_PROJECT% mkdir %CFG_PROJECT%

if "%BOOTLOADER%"=="" set BOOTLOADER=0
if "%CODEC%"=="" set CODEC=0
if "%CODEC_EX%"=="" set CODEC_EX=0
if "%CODEC_IT9910%"=="" set CODEC_IT9910=0
if "%CODEC_EX_IT9910%"=="" set CODEC_EX_IT9910=0
if "%ALT_CPU_IT9850%"=="" set ALT_CPU_IT9850=0
if "%CODEC_IT9850%"=="" set CODEC_IT9850=0
if "%CODEC_IT9xx%"=="" set CODEC_IT9XX=0
if "%ARMLITEDEV%"=="" set ARMLITEDEV=0
if "%ARMLITECODEC%"=="" set ARMLITECODEC=0
if "%TARGET%"=="" set TARGET=%CFG_PROJECT%
if "%AUTOBUILD%"=="" set AUTOBUILD=0
if "%MAKEJOBS%"=="" set /a MAKEJOBS=%NUMBER_OF_PROCESSORS%*2
set BUILD_CMD=post_build.cmd
set RUN_CMD="%CMAKE_SOURCE_DIR%\build\%CFG_PLATFORM%\%CFG_PROJECT%\project\%TARGET%\exec.cmd"
rem # for distcc+
rem set DISTCC_VERBOSE=1
set DISTCC_DIR="%CMAKE_SOURCE_DIR%\build\%CFG_PLATFORM%\%CFG_PROJECT%\"
set TMPDIR="%CMAKE_SOURCE_DIR%\build\%CFG_PLATFORM%\%CFG_PROJECT%\"
rem # for distcc-
set NO_PAUSE=1
set CFG_DEVELOP=0
set CFG_VER_INTERNATIONAL=1
set GNUMAKEFLAGS=--no-print-directory

rem ==========================================================================
rem set AUTOBACKUPCONFIG=1 if you want to backup the .config to the porject dir.
rem ==========================================================================
set AUTOBACKUPCONFIG=0

if "%BOOTLOADER%"=="1" (
    set PRESETTINGS=--loadcfg "%CMAKE_SOURCE_DIR%/build/_presettings/_config_bootloader"
) else (
    set PRESETTINGS=
)

if not "%PRESETTING%"=="" (
    if not "%PRESETTINGS%"=="" (
        set PRESETTINGS=%PRESETTINGS% --loadcfg "%CMAKE_SOURCE_DIR%/build/_presettings/%PRESETTING%"
    ) else (
        set PRESETTINGS=--loadcfg "%CMAKE_SOURCE_DIR%/build/_presettings/%PRESETTING%"
    )
)

if not exist "%CMAKE_SOURCE_DIR%/project/%CFG_PROJECT%/config.cmake" (
    call :restore_config
    if "%AUTOBUILD%"=="1" (
        mconf --autowrite --prefix "CFG_" --cmakefile %CFG_PROJECT%/config.cmake --cfgfile %CFG_PROJECT%/.config %PRESETTINGS% "%CMAKE_SOURCE_DIR%/project/%CFG_PROJECT%/Kconfig"
        call :backup_config
        call post_build.cmd
    ) else (
        call :open_qconf
        call :backup_config
    )
) else (
    call post_build.cmd
)
goto :eof

rem ==========================================================================
rem END OF COMMANDS
rem ==========================================================================

:restore_config
if "%AUTOBACKUPCONFIG%"=="1" (
    if exist "%CMAKE_SOURCE_DIR%/project/%CFG_PROJECT%/.config.%CFG_PLATFORM%" (
        cmake.exe -E copy "%CMAKE_SOURCE_DIR%/project/%CFG_PROJECT%/.config.%CFG_PLATFORM%" %CFG_PROJECT%/.config
    )
)
goto :eof

:backup_config
if "%AUTOBACKUPCONFIG%"=="1" (
    cmake.exe -E copy %CFG_PROJECT%/.config "%CMAKE_SOURCE_DIR%/project/%CFG_PROJECT%/.config.%CFG_PLATFORM%"
)
goto :eof

:open_qconf
rem ==========================================================================
rem open qconf. The qconf must be run from build/xxx/ directory, otherwise
rem some relative path in the kconfig files needs to be fixed.
rem ==========================================================================
if "%AUTOBACKUPCONFIG%"=="1" (
    qconf --fontsize 11 --prefix "CFG_" --cmakefile %CFG_PROJECT%/config.cmake --cfgfile %CFG_PROJECT%/.config %PRESETTINGS% "%CMAKE_SOURCE_DIR%/project/%CFG_PROJECT%/Kconfig"
) else (
    start /b qconf --fontsize 11 --prefix "CFG_" --cmakefile %CFG_PROJECT%/config.cmake --cfgfile %CFG_PROJECT%/.config %PRESETTINGS% "%CMAKE_SOURCE_DIR%/project/%CFG_PROJECT%/Kconfig"
)
goto :eof