@echo off
if not exist build\ mkdir build
pushd build
cl /nologo /Zi /W4 /Femake_icon.exe "%~dp0main.c"
popd
