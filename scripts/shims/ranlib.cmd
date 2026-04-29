@echo off
REM ranlib -> llvm-ranlib shim for PlatformIO native unit tests on Windows.
"C:\Program Files\LLVM\bin\llvm-ranlib.exe" %*
