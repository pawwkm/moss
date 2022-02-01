@echo off
cls

if not exist "bin\" mkdir "bin"
if not exist "bin\obj\" mkdir "bin\obj"

set sources= src/*.c ^
             src/buffers/*.c ^
             src/modes/*.c ^
             src/renderer/*.c
             
set libraries= "%cd%\windows\SDL2-2.0.16\lib\x64\SDL2.lib" ^
               "%cd%\windows\SDL2-2.0.16\lib\x64\SDL2main.lib" ^
               Shell32.lib
               
set includes= -Iwindows\SDL2-2.0.16\include
         
set defines= -D _CRT_SECURE_NO_WARNINGS

set compiler_options= /std:c17 ^
                      /Fobin\obj\  ^
                      /Zi ^
                      /W4 ^
                      /WX ^
                      /wd4701 ^
                      /wd4703 ^
                      /wd4706 ^
                      /wd4201^
                      /diagnostics:caret
                      
set linker_options= /OUT:bin\moss.exe ^
                    /PDB:bin\moss.pdb ^
                    /DEBUG ^
                    /INCREMENTAL:no ^
                    /OPT:ref ^
                    /SUBSYSTEM:windows

cl %sources% %compiler_options% %defines% %includes% /link %linker_options% %libraries%

copy windows\SDL2-2.0.16\lib\x64\SDL2.dll bin\ /y
