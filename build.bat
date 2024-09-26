@echo off
setlocal

:: Change to the directory that the batch file is in
:: %0 = name and path of the current batch file
:: ~d = the drive of the path string (without path and filename)
:: ~p = the path of the current path string (without drive and filename)
:: ~n = the filename of the current path string (without drive and path)
cd /D "%~dp0"

:: Project specific settings
set app_name=c-file-read-bench-win64
set app_version=1.0.0
:: Libraries
set linker_libs=Kernel32.lib User32.lib

:: Source files are relative top the output path which will be \build\[debug or release]\.
set source_files=..\..\src\main.c

:: %a = The current argument name
:: For every argument in the command line arguments create a named variable with the same name and set it to 1
for %%a in (%*) do set "%%a=1"

:: if release is not set make sure debug is set even if omitted
if not "%release%"=="1" set debug=1

:: setup build flags
if "%debug%"=="1" set release=0 && echo [DEBUG BUILD]
if "%release%"=="1" set debug=0 && echo [RELEASE BUILD]

:: MSVC Build options
set common_compiler_flags=/std:c17 /nologo /utf-8 /validate-charset /TC /FC /FAsu /Z7

:: Optimise for AMD with /favor:AMD64
:: Optimise for INTEL with /favor:INTEL64
set special_compiler_flags=/favor:blend /arch:AVX /EHa- /GR- /fp:fast /fp:except- /Oi

set debug_compiler_flags=/Od /GL- /W4
set release_compiler_flags=/O2 /GL- /W4 /WX /DNDEBUG

:: Set disabled warnings
set disabled_warnings=/wd4706

:: Set additional compiler flags
set additional_compiler_flags=

:: Optimize for thread local storage (TLS)
if "%tls%"=="1" set additional_compiler_flags=%additional_compiler_flags% /GA

:: Enable control flow security checks
if "%controlflow%"=="1" set additional_compiler_flags=%additional_compiler_flags% /guard:cf

:: Optimize global data usage: 
if "%globaldata%"=="1" set additional_compiler_flags=%additional_compiler_flags% /Gw

:: Enable function level linking:
if "%fnlink%"=="1" set additional_compiler_flags=%additional_compiler_flags% /Gy

:: Put switch case jump tables in .rdata for maybe a small perf boost
if "%jmprdata%"=="1" set additional_compiler_flags=%additional_compiler_flags% /jumptablerdata

if "%reporting%"=="1" set additional_compiler_flags=%additional_compiler_flags% /Qvec-report:2 /Qpar-report:2 /diagnostics:caret

:: Set linker flags
set common_linker_flags=/INCREMENTAL:NO /MACHINE:x64 /OPT:REF /SUBSYSTEM:CONSOLE
set special_linker_flags=

if "%publish%"=="1" (
    set executable_file=%app_name%-%app_version%.exe
) else (
    set executable_file=%app_name%.exe
)

:: Set compilers
set compile=cl %common_compiler_flags% %special_compiler_flags% %additional_compiler_flags% %disabled_warnings%

:: Prepare output folders
if not exist build mkdir build

pushd build
if "%debug%"=="1" (
    if not exist debug mkdir debug
    set build_path=build\debug
    set compile=%compile% %debug_compiler_flags%
    set debugger=raddbg --auto_step --profile:debug.raddbg_profile
)
if "%release%"=="1" (
    if not exist release mkdir release
    set build_path=build\release
    set compile=%compile% %release_compiler_flags%
    set debugger=raddbg --auto_step --profile:release.raddbg_profile
)
popd

:: Build the product
set compile=%compile% %source_files% /link %common_linker_flags% %special_linker_flags% %linker_libs% /OUT:%executable_file%

pushd %build_path%
call %compile% || exit /b 1
popd

:: Run the product
set app_path="%build_path%\%executable_file%"

if "%rundbg%"=="1" set run=0
if "%run%"=="1" set rundbg=0

if "%run%"=="1" (
    echo [RUNNING APPLICATION]
    call %app_path%
)
if "%rundbg%"=="1" (
    echo [DEBUGGING APPLICATION]
    start /b %debugger%
)

:: Publish the product
if "%publish%"=="1" (
    echo [PUBLISHING APPLICATION]

    pushd build
    if exist publish rmdir publish\ /S /Q
    if not exist publish mkdir publish
    popd

    :: Copy executable
    xcopy /S /V /F /Y "%app_path%" "build\publish\" || exit /b 1
)

:: Unset everything
for %%a in (%*) do set "%%a=0"
set additional_compiler_flags=
set common_compiler_flags=
set special_compiler_flags=
set debug_compiler_flags=
set release_compiler_flags=
set disabled_warnings=
set common_linker_flags=
set special_linker_flags=
set executable_file=
set compile=
set build_path=
set debugger=
set app_path=
set linker_libs=
set source_files=
set app_version=
set app_name=
