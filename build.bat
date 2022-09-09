@echo off
setlocal EnableDelayedExpansion

cls

if not exist "bin\" mkdir "bin"
if exist "bin\obj\" rmdir /S /Q "bin\obj"
mkdir "bin\obj"
del "bin\moss.*" /Q
del "tests\images\*_actual.ppm"
del "tests\images\*_diff.ppm"

set sources= *.c ^
             editor/*.c ^
             editor/buffers/*.c ^
             editor/renderer/*.c

set compiler_options= /std:c17 ^
                      /Fobin\obj\  ^
                      /MP ^
                      /W4 ^
                      /WX ^
                      /wd4701 ^
                      /wd4703 ^
                      /wd4706 ^
                      /wd4201 ^
                      /wd5105 ^
                      /diagnostics:caret

set linker_options= /OUT:bin\moss.exe ^
                    /PDB:bin\moss.pdb ^
                    /INCREMENTAL:no ^
                    /OPT:ref

set defines= -D _CRT_SECURE_NO_WARNINGS ^
             -D WIN32_LEAN_AND_MEAN

if "%1" == "win32" ( 
    set libraries= Shell32.lib ^
                   User32.lib ^
                   Gdi32.lib

    set sources= %sources% ^
                 win32/*.c ^
                 bin/obj/resources.res

    set linker_options= %linker_options% ^
                        /SUBSYSTEM:windows

    rc /fo bin/obj/resources.res win32/resources.rc
)

if "%1" == "tests" ( 
    set libraries=
    set sources= %sources% ^
                 tests/*.c

    set linker_options= %linker_options% ^
                        /SUBSYSTEM:console
)

if "%2" == "release" ( 
    set compiler_options= %compiler_options% ^
                          /Og /Ot /Oy /Ob2 /GF /Gy
) else (
    set compiler_options= %compiler_options% ^
                          /Zi ^
                          /Od ^
                          /fsanitize=address

    set linker_options= %linker_options% ^
                        /DEBUG
)

cl %sources% %compiler_options% %defines% %includes% /link %linker_options% %libraries%
