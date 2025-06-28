@echo off
REM build_linkers.bat - Build Small-C linkers on Windows

echo Building Small-C linkers for Windows...

REM Check if scc.exe exists
if not exist scc.exe (
    echo Error: scc.exe not found. Please build it first.
    exit /b 1
)

REM Detect architecture
set ARCH=%PROCESSOR_ARCHITECTURE%
echo Detected architecture: %ARCH%

REM Build x64 linker
if "%ARCH%"=="AMD64" (
    echo.
    echo Building x64 Windows linker...
    scc.exe sld_win_x64.c > sld_win_x64.s
    if errorlevel 1 goto :error
    
    REM Note: Windows doesn't have 'as', need to use MASM or alternative
    echo Note: You'll need to assemble and link manually using:
    echo   ml64 /c sld_win_x64.s
    echo   link /subsystem:console sld_win_x64.obj syscall_win_x64.obj runtime.obj /out:sld.exe
)

REM Build ARM64 linker
if "%ARCH%"=="ARM64" (
    echo.
    echo Building ARM64 Windows linker...
    scc.exe sld_win_arm64.c > sld_win_arm64.s
    if errorlevel 1 goto :error
    
    echo Note: You'll need to assemble and link manually using ARM64 tools
)

REM Build enhanced linker
echo.
echo Building enhanced Windows linker...
scc.exe sld_win_enhanced.c > sld_win_enhanced.s
if errorlevel 1 goto :error

echo.
echo Assembly files generated successfully!
echo.
echo To complete the build, you need to:
echo 1. Use MASM (ml64) or compatible assembler to assemble the .s files
echo 2. Link with Windows system libraries
echo.
echo Example for x64:
echo   ml64 /c sld_win_x64.s
echo   link sld_win_x64.obj kernel32.lib /out:sld.exe
echo.
echo Or use MinGW/Cygwin tools if available:
echo   as sld_win_x64.s -o sld_win_x64.o
echo   ld sld_win_x64.o syscall_win_x64.o runtime.o -o sld.exe

goto :end

:error
echo Build failed!
exit /b 1

:end
echo.
echo Done!
