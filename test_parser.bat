@echo off
setlocal

if "%~1"=="" (
    echo Usage: %0 ^<grammar_file^> [parser_method]
    echo Example: %0 example_grammar.txt SLR1
    echo Parser methods: LR0, SLR1, CLR1, LALR1
    exit /b 1
)

set GRAMMAR_FILE=%~1
set METHOD=%~2
if "%METHOD%"=="" set METHOD=SLR1

:: Extract file name without extension
for %%F in ("%GRAMMAR_FILE%") do set BASENAME=%%~nF
set OUT_FILE=out_%BASENAME%.json

echo Running %METHOD% parser on %GRAMMAR_FILE%...
type "%GRAMMAR_FILE%" | "%~dp0cpp\bin\lr_lab.exe" %METHOD% > "%OUT_FILE%"

if %ERRORLEVEL% NEQ 0 (
    echo Error running the parser. Please check if the C++ executable is built.
    exit /b 1
)

echo Saved JSON output to %OUT_FILE%
echo Opening %OUT_FILE%...

:: Open the file with the default program (e.g. VS Code, Notepad, or a browser)
start "" "%OUT_FILE%"

endlocal
