#!/bin/sh
# tools/run_bench.sh - compila, roda e valida o benchmark (Linux/POSIX).
#
#   tools/run_bench.sh                  rodada completa -> analysis/
#   tools/run_bench.sh quick            so N = 10000 e 20000
#   tools/run_bench.sh freq_hz=3.6e9    frequencia manual
#   VALIDATE_ONLY=1 tools/run_bench.sh  so valida os arquivos ja gerados
#
# Codigos de saida: 0 = ok, 2 = ok com AVISO de 0 ticks, 1 = erro.

set -u

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd) || exit 1
cd "$ROOT" || exit 1

OUT=analysis
QUICK=0
for arg in "$@"; do
    case "$arg" in
        quick) QUICK=1 ;;
        out=*) OUT=${arg#out=} ;;
    esac
done
# remove barras sobrando no fim, igual ao bench.c
while [ "${#OUT}" -gt 1 ] && [ "${OUT%/}" != "$OUT" ]; do OUT=${OUT%/}; done

build_and_run() {
    if command -v make >/dev/null 2>&1; then
        MAKE=make
    elif command -v gmake >/dev/null 2>&1; then
        MAKE=gmake
    elif command -v mingw32-make >/dev/null 2>&1; then
        MAKE=mingw32-make
    else
        echo "[ERRO] make nao encontrado no PATH" >&2
        exit 1
    fi

    echo "== compilando build/bench.exe =="
    "$MAKE" build/bench.exe || { echo "[ERRO] falha na compilacao" >&2; exit 1; }

    echo
    echo "== rodando build/bench.exe $* =="
    ./build/bench.exe "$@"
    status=$?
    if [ "$status" -ne 0 ]; then
        echo "[ERRO] o benchmark terminou com codigo $status" >&2
        exit "$status"
    fi
}

if [ "${VALIDATE_ONLY:-0}" = "1" ]; then
    echo "== VALIDATE_ONLY=1: so validando '$OUT/' =="
else
    build_and_run "$@"
fi

# ---------------------------------------------------------------- validar
if [ "$QUICK" -eq 1 ]; then
    EXPECTED_N=2
else
    EXPECTED_N=9
fi

echo
echo "== validando $OUT/*.txt =="
FAIL=0
WARN=0
BASE_HEADER=
BASE_NS=
SUMMARY=

for name in sll_insert sll_remove sll_list \
            vector_insert vector_remove vector_list \
            avl_insert avl_remove avl_list; do
    file="$OUT/$name.txt"

    if [ ! -f "$file" ]; then
        echo "[ERRO] arquivo ausente: $file" >&2
        FAIL=1
        continue
    fi

    # 1) mesma maquina / mesma rodada: a 2a linha (cpu/freq/os) tem que ser igual
    header=$(sed -n '2p' "$file")
    if [ -z "$BASE_HEADER" ]; then
        BASE_HEADER=$header
    elif [ "$header" != "$BASE_HEADER" ]; then
        echo "[ERRO] $name.txt: cabecalho diferente dos outros (rodadas misturadas?)" >&2
        echo "        esperado: $BASE_HEADER" >&2
        echo "        obtido:   $header" >&2
        FAIL=1
    fi

    # 2) mesmos N nos 9 arquivos, na mesma ordem, e 4 colunas por linha
    ns=$(awk -F, '
        /^[[:space:]]*#/ { next }
        { if (NF != 4) bad = 1; n[++c] = $1 }
        END {
            if (bad) { print "BADCOLS"; exit }
            for (i = 1; i <= c; i++)
                printf "%s%s", (i > 1 ? ";" : ""), n[i]
        }
    ' "$file")

    if [ "$ns" = "BADCOLS" ]; then
        echo "[ERRO] $name.txt: alguma linha de dados nao tem 4 colunas" >&2
        FAIL=1
    elif [ -z "$ns" ]; then
        echo "[ERRO] $name.txt: nenhuma linha de dados" >&2
        FAIL=1
    elif [ "$(printf '%s\n' "$ns" | tr ';' '\n' | wc -l)" -ne "$EXPECTED_N" ]; then
        echo "[ERRO] $name.txt: esperava $EXPECTED_N pontos de N, encontrei $ns" >&2
        FAIL=1
    elif [ -z "$BASE_NS" ]; then
        BASE_NS=$ns
    elif [ "$ns" != "$BASE_NS" ]; then
        echo "[ERRO] $name.txt: N diferente dos outros arquivos ($ns != $BASE_NS)" >&2
        FAIL=1
    fi

    # 3) nunca zero (o graphs.py usa log)
    if awk -F, '!/^[[:space:]]*#/ && ($2 <= 0 || $3 <= 0 || $4 <= 0) { bad = 1 } END { exit !bad }' "$file"; then
        echo "[ERRO] $name.txt: valor <= 0 (quebraria o log no graphs.py)" >&2
        FAIL=1
    fi

    # 4) aviso de 0 ticks (limite superior, tipico do clock() do Windows)
    if grep -q '^# AVISO' "$file"; then
        echo "[AVISO] $name.txt: algum ponto ficou no limite do clock() (1 tick/operacoes)" >&2
        WARN=1
    fi

    SUMMARY="$SUMMARY$name $(awk -F, '!/^[[:space:]]*#/ { printf "%s:%.0f  ", $1, $2 }' "$file")
"
done

echo
echo "== resumo (n:mediana_ciclos) =="
printf '%s' "$SUMMARY"

if [ "$FAIL" -ne 0 ]; then
    echo
    echo "[ERRO] validacao falhou" >&2
    exit 1
fi

echo
if [ "$WARN" -ne 0 ]; then
    echo "[OK] 9 arquivos validos, mas veja os AVISOs acima (resolucao do clock())."
    exit 2
fi

echo "[OK] 9 arquivos validos em '$OUT/': mesma rodada, mesmos N, sem zeros."
