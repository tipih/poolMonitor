@echo off
REM gcc -> clang shim for PlatformIO native unit tests on Windows.
REM Forwards every argument to clang, which lives in C:\Program Files\LLVM\bin.
"C:\Program Files\LLVM\bin\clang.exe" %*
