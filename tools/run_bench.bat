@echo off
rem tools\run_bench.bat -- compila, roda e valida o benchmark (Windows).
rem
rem   tools\run_bench.bat                        rodada completa  -> analysis\
rem   tools\run_bench.bat quick                  so N = 10000 e 20000
rem   tools\run_bench.bat freq_hz=3.6e9          frequencia manual
rem   tools\run_bench.bat out=analysis quick     out= e quick em qualquer ordem
rem   set VALIDATE_ONLY=1 ^&^& tools\run_bench.bat  nao compila nem roda: so valida
rem
rem Faz, a partir da RAIZ do projeto (nao importa de onde voce chama):
rem   1. compila com "mingw32-make build/bench.exe" (sempre -O0, via Makefile)
rem   2. roda build\bench.exe repassando os argumentos
rem   3. valida os 9 arquivos: existem, mesma maquina/rodada, mesmos N,
rem      nenhum zero, e mostra um resumo no terminal
rem Codigos de saida: 0 = tudo ok, 2 = ok mas com AVISO de 0 ticks,
rem                   1 = erro (compilacao, execucao ou validacao).
rem
rem Obs.: cmd/PowerShell NAO expandem "src\DataTypes\*\*.c", por isso aqui a
rem compilacao vai sempre pelo Makefile (que usa $(wildcard)). Para compilar
rem na mao, liste os arquivos um por um: veja o README/comentario do bench.c.

setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0.." || goto :fail

set "OUT=analysis"
set "QUICK=0"

rem cmd nao aceita "for %%A in ()" (sem argumentos), por isso o desvio:
if "%~1"=="" goto :args_done

:parse_args
for %%A in (%*) do (
    set "ARG=%%~A"
    if /i "!ARG!"=="quick" set "QUICK=1"
    if /i "!ARG:~0,4!"=="out=" set "OUT=!ARG:~4!"
)

:args_done

rem remove barras sobrando no fim ("analysis\" -> "analysis"), igual ao bench.c
if "%OUT:~-1%"=="/" set "OUT=%OUT:~0,-1%"
if "%OUT:~-1%"=="\" set "OUT=%OUT:~0,-1%"

if "%VALIDATE_ONLY%"=="1" goto :validate

rem ------------------------------------------------------------------ compilar
set "MK="
where mingw32-make >nul 2>&1
if not errorlevel 1 set "MK=mingw32-make"
if not defined MK (
    where make >nul 2>&1
    if not errorlevel 1 set "MK=make"
)

if not defined MK (
    if exist "build\bench.exe" (
        echo [AVISO] nem mingw32-make nem make no PATH: usando build\bench.exe existente
        goto :run
    )
    echo [ERRO] nem mingw32-make nem make encontrados no PATH
    goto :fail
)

echo == compilando build\bench.exe ==
%MK% build/bench.exe
if errorlevel 1 (
    echo [ERRO] falha na compilacao
    goto :fail
)

:run
echo.
echo == rodando build\bench.exe %* ==
build\bench.exe %*
if errorlevel 1 (
    echo [ERRO] o benchmark terminou com codigo %ERRORLEVEL%
    goto :fail
)

rem ------------------------------------------------------------------- validar
:validate
set "EXPECTED=11"
set "NLIST=10000 20000 30000 40000 50000 60000 80000 90000 100000 125000 150000"
if "%QUICK%"=="1" (
    set "EXPECTED=2"
    set "NLIST=10000 20000"
)

echo.
echo == validando %OUT%\*.txt ==
set "FAIL=0"
set "WARN=0"
set "BASEH="
set "TMPD=%TEMP%"
if not defined TMPD set "TMPD=."
set "SUMFILE=%TMPD%\bench_validacao.txt"
if exist "%SUMFILE%" del /q "%SUMFILE%"

for %%N in (sll_insert sll_remove sll_list vector_insert vector_remove vector_list avl_insert avl_remove avl_list) do call :check %%N

echo.
echo == resumo ^(n:mediana_ciclos^) ==
if exist "%SUMFILE%" type "%SUMFILE%"
if exist "%SUMFILE%" del /q "%SUMFILE%"

if "%FAIL%"=="1" goto :fail

echo.
if "%WARN%"=="1" (
    echo [OK] 9 arquivos validos, mas veja os AVISOs acima ^(resolucao do clock^).
    popd
    endlocal
    exit /b 2
)

echo [OK] 9 arquivos validos em "%OUT%": mesma rodada, mesmos N, sem zeros.
popd
endlocal
exit /b 0

:fail
echo.
echo [ERRO] validacao falhou
popd
endlocal
exit /b 1

rem ===========================================================================
rem :check <nome>  -- valida um arquivo; acumula em FAIL/WARN e no resumo
rem ===========================================================================
:check
set "NAME=%~1"
set "FILE=%OUT%\%NAME%.txt"

if not exist "%FILE%" (
    echo [ERRO] arquivo ausente: %FILE%
    set "FAIL=1"
    goto :eof
)

rem 1) mesma maquina / mesma rodada: a 2a linha (cpu/freq/os) tem que ser igual
set "H="
for /f "usebackq skip=1 delims=" %%H in ("%FILE%") do if not defined H set "H=%%H"
if not defined BASEH set "BASEH=%H%"
findstr /L /C:"%BASEH%" "%FILE%" >nul 2>&1
if errorlevel 1 (
    echo [ERRO] %NAME%.txt: cabecalho diferente dos outros ^(rodadas misturadas?^)
    echo         esperado: %BASEH%
    echo         obtido:   %H%
    set "FAIL=1"
)

rem 2) os N esperados, cada um exatamente uma vez (mesmos N nos 9 arquivos)
set "CNT=0"
for %%N in (!NLIST!) do (
    findstr /R /B /C:"^%%N," "%FILE%" >nul 2>&1
    if errorlevel 1 (
        echo [ERRO] %NAME%.txt: falta a linha de n=%%N
        set "FAIL=1"
    ) else (
        set /a CNT+=1
    )
)
for /f %%c in ('findstr /R /B /C:"^[0-9]" "%FILE%" ^| find /c /v ""') do set "CNT2=%%c"
if not "!CNT!"=="!EXPECTED!" (
    echo [ERRO] %NAME%.txt: esperava !EXPECTED! pontos de N
    set "FAIL=1"
)
if not "!CNT2!"=="!EXPECTED!" (
    echo [ERRO] %NAME%.txt: encontrei !CNT2! linhas de dados
    set "FAIL=1"
)

rem 3) nunca zero (o graphs.py usa log)
findstr /C:",0.0," "%FILE%" >nul 2>&1
if not errorlevel 1 (
    echo [ERRO] %NAME%.txt: valor 0.0 ^(quebraria o log no graphs.py^)
    set "FAIL=1"
)

rem 4) aviso de 0 ticks (limite superior, tipico do clock do Windows)
findstr /B /C:"# AVISO" "%FILE%" >nul 2>&1
if not errorlevel 1 (
    echo [AVISO] %NAME%.txt: algum ponto ficou no limite do clock ^(1 tick/operacoes^)
    set "WARN=1"
)

rem resumo do arquivo
set "SUM="
for /f "usebackq tokens=1,2 delims=," %%a in (`findstr /V /B "#" "%FILE%"`) do set "SUM=!SUM!%%a:%%b  "
>>"%SUMFILE%" echo %NAME% !SUM!

goto :eof
