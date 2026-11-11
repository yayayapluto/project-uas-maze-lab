@echo off
rem Build Maze Lab dengan MinGW-w64 gcc; menghasilkan maze_lab.exe di root.
setlocal
set CC=gcc
set CFLAGS=-std=c99 -Wall -Wextra -Wpedantic -g
set LDLIBS=-lm
set TARGET=maze_lab.exe

if "%1"=="asan" goto asan
if "%1"=="clean" goto clean
if "%1"=="test" goto test

%CC% %CFLAGS% -o %TARGET% common\common.c maze_core\maze_core.c maze_core\maze_validate.c maze_gen\maze_gen.c maze_solve\maze_solve.c maze_editor\maze_editor.c maze_editor\maze_editor_cmd.c maze_catalog\maze_catalog.c maze_bench\maze_bench.c maze_bench\maze_bench_report.c maze_lab\maze_lab.c maze_lab\maze_catalog_menu.c maze_lab\maze_bench_menu.c maze_test\maze_test.c maze_test\maze_test_extra.c maze_main.c %LDLIBS%
if errorlevel 1 exit /b 1
echo Build selesai: %TARGET%
exit /b 0

:asan
%CC% %CFLAGS% -fsanitize=address,undefined -o maze_lab_asan.exe common\common.c maze_core\maze_core.c maze_core\maze_validate.c maze_gen\maze_gen.c maze_solve\maze_solve.c maze_editor\maze_editor.c maze_editor\maze_editor_cmd.c maze_catalog\maze_catalog.c maze_bench\maze_bench.c maze_bench\maze_bench_report.c maze_lab\maze_lab.c maze_lab\maze_catalog_menu.c maze_lab\maze_bench_menu.c maze_test\maze_test.c maze_test\maze_test_extra.c maze_main.c %LDLIBS%
if errorlevel 1 exit /b 1
echo Build ASan selesai: maze_lab_asan.exe
exit /b 0

:test
%CC% %CFLAGS% -o %TARGET% common\common.c maze_core\maze_core.c maze_core\maze_validate.c maze_gen\maze_gen.c maze_solve\maze_solve.c maze_editor\maze_editor.c maze_editor\maze_editor_cmd.c maze_catalog\maze_catalog.c maze_bench\maze_bench.c maze_bench\maze_bench_report.c maze_lab\maze_lab.c maze_lab\maze_catalog_menu.c maze_lab\maze_bench_menu.c maze_test\maze_test.c maze_test\maze_test_extra.c maze_main.c %LDLIBS%
if errorlevel 1 exit /b 1
%TARGET% --test
exit /b %errorlevel%

:clean
del /q %TARGET% maze_lab_asan.exe *.o common\*.o 2>nul
echo Bersih selesai.
exit /b 0

