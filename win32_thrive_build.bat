@echo off

REM ---------------------------------------------------------------------------
REM GCC/Clang Build
REM ---------------------------------------------------------------------------
REM
set DEF_COMPILER_FLAGS=-std=c89 -pedantic -nodefaultlibs -nostdlib ^
-mconsole -march=native -mtune=native -mno-stack-arg-probe ^
-Xlinker /STACK:0x100000,0x100000 ^
-Xlinker /ENTRY:nostdlib_main ^
-fno-builtin -ffreestanding -fno-asynchronous-unwind-tables -fuse-ld=lld ^
-Wall -Wextra -Werror -Wvla -Wconversion -Wdouble-promotion -Wsign-conversion ^
-Wmissing-field-initializers -Wuninitialized -Winit-self ^
-Wunused -Wunused-macros -Wunused-local-typedefs

set DEF_FLAGS_LINKER=-lkernel32

REM ---------------------------------------------------------------------------
REM Build Targets
REM ---------------------------------------------------------------------------
cc -s -O2 %DEF_COMPILER_FLAGS% win32_thrive.c -o win32_thrive.exe %DEF_FLAGS_LINKER%

REM ---------------------------------------------------------------------------
REM Run
REM ---------------------------------------------------------------------------
win32_thrive.exe hello.thrive

REM ---------------------------------------------------------------------------
REM Assemble
REM ---------------------------------------------------------------------------
nasm -f win64 thrive.asm

REM ---------------------------------------------------------------------------
REM Link
REM ---------------------------------------------------------------------------
cc -s -nostdlib thrive.obj -o thrive.exe -lkernel32
