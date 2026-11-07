/*
 * main.c - Benchmark das 3 estruturas (Passos 6 e 7 do enunciado)
 *
 * Compilar (sem otimizacao, como exige o Passo 8):
 *     gcc -O0 main.c timestamp.c vector.c slinkedlist.c -o programa
 *     (quando a AVL estiver pronta: defina USAR_AVL como 1 e acrescente avl.c)
 *
 * Executar:
 *     ./programa
 *
 * Saidas:
 *     - tela (stderr): metadados e tabela de resultados
 *     - resultados.csv: estrutura, N, operacao, mediana, min e max em ciclos
 *
 * O stdout e redirecionado para o dispositivo nulo logo no inicio, porque as
 * funcoes de listagem usam printf e o custo de escrever no terminal
 * distorceria a medicao. Por isso todo o texto do programa vai para stderr.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* AVL ainda nao implementada: 0 = desligada, 1 = ligada (tambem exige avl.h e avl.c). */
#ifndef USAR_AVL
#define USAR_AVL 0
#endif

#include "timestamp.h"
#include "Vector.h"
#include "SinglyLInkedList.h"   /* ADAPTE: nome do header da lista ligada */
#if USAR_AVL
#include "avl.h"           /* ADAPTE: nome do header da AVL          */
#endif

/* ===================== CONFIGURACAO ===================== */

#define ARQUIVO_DADOS      "data/timestamps_100k.txt"  /* "/" funciona no Windows tambem */
#define ARQUIVO_CSV        "resultados.csv"
#define REPETICOES         10                          /* Passo 7 */
#define VETOR_CAP_INICIAL  16

/* PREENCHER: registre os mesmos valores na metodologia do relatorio (Passo 6) */
#define CPU_DESCRICAO      "PREENCHER: modelo exato do processador"
#define FREQ_BASE_HZ       3.0e9                       /* PREENCHER: frequencia base nominal, em Hz */

#ifdef _WIN32
#define DISPOSITIVO_NULO   "NUL"
#else
#define DISPOSITIVO_NULO   "/dev/null"
#endif

/*
 * Tamanhos testados (Passo 5). Cada N usa os N primeiros timestamps do
 * arquivo, na ordem em que estao (mesma entrada, mesma ordem, para as 3
 * estruturas).
 */
#define NUM_TAMANHOS 5
static const int TAMANHOS[NUM_TAMANHOS] = { 1000, 5000, 10000, 50000, 100000 };
#define N_MAX 100000

/*
 * Lote adaptativo: o clock() tem resolucao baixa (cerca de 1 ms no Windows),
 * entao uma operacao rapida pode durar menos que um tick e medir 0. Antes das
 * 10 repeticoes, calibrar_lote() procura quantas execucoes independentes
 * (em estruturas separadas, uma apos a outra) sao necessarias para a medicao
 * durar pelo menos TEMPO_MIN_SEG. Operacoes lentas ficam com lote 1.
 * O valor reportado e sempre o custo de UMA execucao completa (inserir N,
 * remover N ou listar uma vez): o tempo total e dividido pelo lote.
 *
 * REGISTROS_LOTE_MAX limita lote * N, para nao gastar memoria demais.
 */
#define TEMPO_MIN_SEG        0.020
#define REGISTROS_LOTE_MAX   2000000L

/* ===================== ADAPTADORES ===================== */
/*
 * Interface unica para as 3 estruturas. Se os nomes das suas funcoes forem
 * diferentes, e so mudar o corpo dos adaptadores abaixo.
 * Convencao esperada: inserir e remover retornam 0 em sucesso.
 */
typedef struct {
    const char *nome;                         /* sem virgulas (vai para o CSV) */
    void *(*criar)(void);
    int   (*inserir)(void *s, time_t t);
    int   (*remover)(void *s, time_t t);
    void  (*listar)(void *s);                 /* imprime em ordem ascendente */
    void  (*destruir)(void *s);
} Estrutura;

/* ---- Lista ligada simples (pre-ordenacao na listagem) ---- */
static void *lista_criar(void)                 { return slCreate(); }          /* ADAPTE */
static int   lista_inserir(void *s, time_t t)  { return slInsert(s, t); }      /* ADAPTE: deve retornar 0 em sucesso */
static int   lista_remover(void *s, time_t t)  { return slRemove(s, t) ? 0 : -1; } /* slRemove retorna 1 em sucesso */
static void  lista_listar(void *s)             { slListAscending(s); }         /* ADAPTE: deve imprimir em ordem ascendente */
static void  lista_destruir(void *s)           { slDestroy(s); }               /* ADAPTE */

/* ---- Vetor ordenado (insercao ordenada) ---- */
static void *vec_criar(void)                   { return vCreate(VETOR_CAP_INICIAL); }
static int   vec_inserir(void *s, time_t t)    { return vInsert(s, t); }
static int   vec_remover(void *s, time_t t)    { return vRemove(s, t); }
static void  vec_listar(void *s)               { vListAscending(s); }
static void  vec_destruir(void *s)             { vDestroy(s); }

#if USAR_AVL
/* ---- AVL (insercao ordenada) ---- */
static void *avl_criar(void)                   { return avlCreate(); }         /* ADAPTE */
static int   avl_inserir(void *s, time_t t)    { return avlInsert(s, t); }     /* ADAPTE */
static int   avl_remover(void *s, time_t t)    { return avlRemove(s, t); }     /* ADAPTE */
static void  avl_listar(void *s)               { avlListAscending(s); }        /* ADAPTE */
static void  avl_destruir(void *s)             { avlDestroy(s); }              /* ADAPTE */
#endif

#if USAR_AVL
#define NUM_ESTRUTURAS 3
#else
#define NUM_ESTRUTURAS 2
#endif
static const Estrutura ESTRUTURAS[NUM_ESTRUTURAS] = {
    { "Lista ligada (pre-ordenacao)", lista_criar, lista_inserir, lista_remover, lista_listar, lista_destruir },
    { "Vetor ordenado",               vec_criar,   vec_inserir,   vec_remover,   vec_listar,   vec_destruir   },
#if USAR_AVL
    { "AVL",                          avl_criar,   avl_inserir,   avl_remover,   avl_listar,   avl_destruir   },
#endif
};

/* ===================== MEDICAO ===================== */

typedef enum { OP_INSERIR, OP_REMOVER, OP_LISTAR, NUM_OPS } Operacao;
static const char *NOMES_OPS[NUM_OPS] = { "inserir", "remover", "listar_ordenado" };

static void abortar(const char *msg)
{
    fprintf(stderr, "ERRO: %s\n", msg);
    exit(1);
}

/*
 * Mede a operacao 'op' sobre os 'n' primeiros dados, executada em 'lote'
 * estruturas independentes, e devolve os SEGUNDOS DE CPU POR EXECUCAO
 * (tempo total medido com clock() dividido por 'lote'). A conversao para
 * ciclos (Passo 6) e feita em para_ciclos():
 *     ciclos = ((clock_f - clock_i) / CLOCKS_PER_SEC) * freq_base
 *
 *  - OP_INSERIR: insere os n registros, na ordem do arquivo, numa estrutura vazia.
 *  - OP_REMOVER: estrutura ja contendo os n registros; remove todos, na ordem do arquivo.
 *  - OP_LISTAR : estrutura ja contendo os n registros; uma chamada de listar_ordenado.
 *
 * Criacao, preparacao e destruicao ficam FORA do intervalo medido.
 */
static double medir(const Estrutura *e, Operacao op, const time_t *dados, int n, int lote)
{
    void **est = malloc((size_t)lote * sizeof *est);
    if (est == NULL)
        abortar("memoria insuficiente");

    for (int k = 0; k < lote; k++) {
        est[k] = e->criar();
        if (est[k] == NULL)
            abortar("falha ao criar a estrutura");

        if (op != OP_INSERIR) {                       /* preparacao (nao medida) */
            for (int i = 0; i < n; i++)
                if (e->inserir(est[k], dados[i]) != 0)
                    abortar("falha na insercao durante a preparacao");
        }
    }

    long falhas = 0;
    clock_t inicio = clock();

    switch (op) {
    case OP_INSERIR:
        for (int k = 0; k < lote; k++)
            for (int i = 0; i < n; i++)
                if (e->inserir(est[k], dados[i]) != 0)
                    falhas++;
        break;
    case OP_REMOVER:
        for (int k = 0; k < lote; k++)
            for (int i = 0; i < n; i++)
                if (e->remover(est[k], dados[i]) != 0)
                    falhas++;
        break;
    case OP_LISTAR:
        for (int k = 0; k < lote; k++)
            e->listar(est[k]);
        break;
    default:
        break;
    }

    clock_t fim = clock();

    for (int k = 0; k < lote; k++)
        e->destruir(est[k]);
    free(est);

    if (falhas > 0)
        abortar("operacao retornou falha durante a medicao");

    return ((double)(fim - inicio) / CLOCKS_PER_SEC) / lote;   /* segundos por execucao */
}

/* Converte segundos de CPU em ciclos estimados (equacao do Passo 6). */
static double para_ciclos(double segundos)
{
    return segundos * FREQ_BASE_HZ;
}

/* Escolhe o menor lote (potencia de 4) cuja medicao dura pelo menos TEMPO_MIN_SEG. */
static int calibrar_lote(const Estrutura *e, Operacao op, const time_t *dados, int n)
{
    int lote_max = (int)(REGISTROS_LOTE_MAX / n);
    if (lote_max < 1)
        lote_max = 1;

    int lote = 1;
    while (lote < lote_max) {
        double total = medir(e, op, dados, n, lote) * lote;
        if (total >= TEMPO_MIN_SEG)
            break;
        lote *= 4;
        if (lote > lote_max)
            lote = lote_max;
    }
    return lote;
}

/* ===================== ESTATISTICA (Passo 7) ===================== */

static int cmp_double(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

/* Ordena v e devolve mediana (media do 5o e 6o de 10 valores), minimo e maximo. */
static void estatisticas(double *v, int n, double *mediana, double *minimo, double *maximo)
{
    qsort(v, (size_t)n, sizeof *v, cmp_double);
    *minimo  = v[0];
    *maximo  = v[n - 1];
    *mediana = (v[n / 2 - 1] + v[n / 2]) / 2.0;      /* n = 10 -> v[4] e v[5] */
}

/* ===================== DADOS ===================== */

/* Le um timestamp por linha e converte para time_t. Retorna a quantidade lida, ou -1 em erro. */
static int carregar_dados(const char *caminho, time_t *dados, int max)
{
    FILE *f = fopen(caminho, "r");
    if (f == NULL) {
        perror(caminho);
        return -1;
    }

    char linha[128];
    int n = 0, numero_linha = 0;

    while (n < max && fgets(linha, sizeof linha, f) != NULL) {
        numero_linha++;
        linha[strcspn(linha, "\r\n")] = '\0';        /* aceita arquivos com CRLF */
        if (linha[0] == '\0')
            continue;                                /* ignora linhas vazias */

        if (timestamp_parse(linha, &dados[n]) != 0) {
            fprintf(stderr, "ERRO: timestamp invalido na linha %d: \"%s\"\n", numero_linha, linha);
            fclose(f);
            return -1;
        }
        n++;
    }

    fclose(f);
    return n;
}

/* ===================== MAIN ===================== */

int main(void)
{
    /* Descarta o stdout: as funcoes de listagem usam printf. */
    if (freopen(DISPOSITIVO_NULO, "w", stdout) == NULL) {
        fprintf(stderr, "ERRO: nao foi possivel redirecionar stdout para %s\n", DISPOSITIVO_NULO);
        return 1;
    }

    static time_t dados[N_MAX];
    int total = carregar_dados(ARQUIVO_DADOS, dados, N_MAX);
    if (total < 0)
        return 1;
    if (total < N_MAX) {
        fprintf(stderr, "ERRO: %s tem %d registros, mas sao necessarios %d\n",
                ARQUIVO_DADOS, total, N_MAX);
        return 1;
    }

    FILE *csv = fopen(ARQUIVO_CSV, "w");
    if (csv == NULL) {
        perror(ARQUIVO_CSV);
        return 1;
    }
    fprintf(csv, "estrutura,N,operacao,mediana_ciclos,min_ciclos,max_ciclos,lote\n");

    fprintf(stderr, "Processador : %s\n", CPU_DESCRICAO);
    fprintf(stderr, "Freq. base  : %.0f Hz\n", (double)FREQ_BASE_HZ);
    fprintf(stderr, "CLOCKS_PER_SEC = %ld\n", (long)CLOCKS_PER_SEC);
    fprintf(stderr, "Dados       : %s (%d registros)\n", ARQUIVO_DADOS, total);
    fprintf(stderr, "Repeticoes  : %d (mediana = media do 5o e 6o valores ordenados)\n\n", REPETICOES);

    for (int t = 0; t < NUM_TAMANHOS; t++) {
        int n = TAMANHOS[t];

        for (int s = 0; s < NUM_ESTRUTURAS; s++) {
            const Estrutura *e = &ESTRUTURAS[s];
            fprintf(stderr, "[N=%d] %s\n", n, e->nome);

            for (int op = 0; op < NUM_OPS; op++) {
                int lote = calibrar_lote(e, (Operacao)op, dados, n);

                double ciclos[REPETICOES];
                for (int r = 0; r < REPETICOES; r++)
                    ciclos[r] = para_ciclos(medir(e, (Operacao)op, dados, n, lote));

                double med, min, max;
                estatisticas(ciclos, REPETICOES, &med, &min, &max);

                fprintf(stderr, "    %-16s mediana=%.0f  min=%.0f  max=%.0f  (lote=%d)\n",
                        NOMES_OPS[op], med, min, max, lote);
                fprintf(csv, "%s,%d,%s,%.0f,%.0f,%.0f,%d\n",
                        e->nome, n, NOMES_OPS[op], med, min, max, lote);
                fflush(csv);
            }
        }
        fprintf(stderr, "\n");
    }

    fclose(csv);
    fprintf(stderr, "Concluido. Resultados em %s\n", ARQUIVO_CSV);
    return 0;
}