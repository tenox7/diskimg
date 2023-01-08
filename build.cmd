set OPATH=%PATH%
set VCVARS="C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat"
del *.exe *.obj

set INCLUDE= & set LIB= & set LIBPATH= & PATH=%OPATH%
call %VCVARS% amd64
forfiles /m *.c /c "cmd /c cl /Fe@fname-x64 @file"

set INCLUDE= & set LIB= & set LIBPATH= & PATH=%OPATH%
call %VCVARS% amd64_x86
forfiles /m *.c /c "cmd /c cl /Fe@fname-x86 @file"

set INCLUDE= & set LIB= & set LIBPATH= & PATH=%OPATH%
call %VCVARS% amd64_arm
forfiles /m *.c /c "cmd /c cl /D_ARM_WINAPI_PARTITION_DESKTOP_SDK_AVAILABLE /Fe@fname-arm @file"

set INCLUDE= & set LIB= & set LIBPATH= & PATH=%OPATH%
call %VCVARS% amd64_arm64
forfiles /m *.c /c "cmd /c cl /D_ARM_WINAPI_PARTITION_DESKTOP_SDK_AVAILABLE /Fe@fname-arm64 @file"

del *.obj
set INCLUDE= & set LIB= & set LIBPATH= & PATH=%OPATH%
