@echo off

set common_compiler_flags=-Od -fp:fast -nologo -MD -Oi -Gm- -GR- -EHa- -W4 -WX -wd4201 -wd4100 -wd4189 -wd4505 -wd4456 -wd4459 -wd4311 -wd4312 -wd4302 -wd4706 -wd4127 -FC -Z7

IF NOT EXIST bin mkdir bin
pushd bin

cl %common_compiler_flags% ..\src\chip8.cpp /link -incremental:no -opt:ref ..\lib\raylib.lib user32.lib gdi32.lib winmm.lib shell32.lib

popd
