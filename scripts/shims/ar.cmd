@echo off
REM ar -> llvm-ar shim for PlatformIO native unit tests on Windows.
"C:\Program Files\LLVM\bin\llvm-ar.exe" %*
