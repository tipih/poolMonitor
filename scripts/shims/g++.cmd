@echo off
REM g++ -> clang++ shim for PlatformIO native unit tests on Windows.
"C:\Program Files\LLVM\bin\clang++.exe" %*
