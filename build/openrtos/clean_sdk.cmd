rem @ECHO OFF
PUSHD .
DIR /ad /b > cleandir
FOR /F "usebackq" %%i IN (cleandir) DO (
    IF NOT %%i == arm_lite_codec (
        IF NOT %%i == lib (
            rd /s /q %%i
        )
    )
)
DEL cleandir

cd ../win32
DIR /ad /b > cleandir
FOR /F "usebackq" %%i IN (cleandir) DO (
    IF NOT %%i == lib (
        rd /s /q %%i
    )
)
DEL cleandir
POPD