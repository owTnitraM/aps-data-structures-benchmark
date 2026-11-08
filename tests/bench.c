/*
 * tests/bench.c -- benchmark das 3 estruturas do catalogo de timestamps
 *
 * Para cada estrutura (SLL, Vector, AVL) e cada operacao obrigatoria
 * (inserir, remover, listar_ordenado) mede o custo em CICLOS de CPU por
 * operacao em funcao de N e escreve 9 arquivos de texto em <out>/ (padrao
 * analysis/) que o tools/graphs.py carrega com np.loadtxt(..., comments='#').
 *
 * PROTOCOLO DE MEDICAO (decisoes)
 *
 *  - N = 1000, 5000, 10000, 50000, 100000.  Com o argumento "quick" somente
 *    N = 1000 e 5000. Os mesmos valores de N (na mesma ordem) vao para os
 *    9 arquivos, para o custo ponderado 70/15/15 poder ser calculado depois.
 *
 *  - Dados: data/timestamps_100k.txt (se nao existir, tenta Data/..., que e
 *    como a pasta esta neste repositorio) e lido UMA vez para um vetor de
 *    time_t, com fgets + strip de \r\n + timestamp_parse. O generator.py ja
 *    grava o arquivo fora de ordem cronologica (ele verifica isso com
 *    assert), mas mesmo assim embaralhamos com xorshift64 de semente fixa
 *    (rand() nao: varia entre plataformas), para garantir que a ordem de
 *    chegada != ordem cronologica e que as 3 estruturas recebam exatamente a
 *    mesma sequencia, na mesma ordem, em qualquer sistema.
 *
 *  - Um ponto = custo de UMA operacao sobre uma estrutura de tamanho ~N:
 *    uma insercao, uma remocao ou uma listagem completa. Valor do ponto =
 *    ciclos cronometrados acumulados / total de operacoes.
 *
 *  - clock() tem resolucao grossa (no Windows ~1 ms ou pior) e uma operacao
 *    dura nanossegundos. Cada repeticao funciona assim:
 *      1. a resolucao do clock() e medida no inicio (t0=clock(); while
 *         (clock()==t0);) e registrada no cabecalho dos arquivos;
 *      2. lote de K = max(1, N/20) operacoes (na listagem, K = 1: uma
 *         listagem completa), mantendo o tamanho da estrutura em +/-5% de N;
 *      3. rodadas: monta a base (NAO cronometrado), cronometra o lote,
 *         reconstroi a base (NAO cronometrado); soma so os intervalos
 *         cronometrados ate >= TARGET_TICKS ticks de clock() (padrao 100)
 *         ou ate MAX_SECONDS_PER_POINT (padrao 2 s), com uma trava extra
 *         pelo relogio para nao entrar em laco infinito quando cada rodada
 *         custa menos de 1 tick;
 *      4. se o acumulado ainda for 0 ticks, grava 1 tick / total de
 *         operacoes (limite superior), avisa no stderr e escreve uma linha
 *         "# AVISO" no cabecalho do arquivo;
 *      5. uma rodada de aquecimento descartada antes das 10 repeticoes.
 *
 *    DIFERENCA DELIBERADA em relacao ao roteiro ("desfazer o lote"): a base
 *    e RECONSTRUIDA a cada rodada em vez de remover/reinserir o lote. Na SLL,
 *    reinserir no inicio muda a ordem dos nos a cada rodada e enviesaria a
 *    busca linear das rodadas seguintes; reconstruir garante que todas as
 *    rodadas usem a mesma base/distribuicao. O custo de reconstrucao fica
 *    FORA do cronometro.
 *
 *  - 10 repeticoes por ponto. Valor representativo = MEDIANA (media do 5o e
 *    do 6o valores depois de ordenar); minimo e maximo tambem sao gravados.
 *
 *  - ciclos = (ticks / CLOCKS_PER_SEC) * freq_hz / operacoes. Sempre usa a
 *    macro CLOCKS_PER_SEC. No Linux clock() mede tempo de CPU do processo
 *    (resolucao de microssegundos); no Windows mede tempo de relogio, com
 *    resolucao bem pior -- por isso o protocolo de lote/acumulo.
 *
 *  - A listagem da SLL (slListAscending) ordena a lista no lugar. Como a base
 *    e reconstruida antes de cada rodada, isso nao contamina as medidas.
 *
 *  - stdout e redirecionado para NUL (Windows) / /dev/null (Linux) antes de
 *    qualquer medicao: as listagens nunca devem medir o terminal. Todo log
 *    de progresso vai para o stderr. Nada e escrito no stdout.
 *
 *  - Compilar SEMPRE com -O0 (nenhuma otimizacao), pois o objetivo e medir a
 *    estrutura, nao o otimizador.
 *
 * TEMPO ESTIMADO (medido): a rodada COMPLETA (5 valores de N, 3 estruturas,
 * 3 operacoes, 10 repeticoes + aquecimento) levou ~38 s num i9-13900H a
 * 2,6 GHz (Linux, clock() de 1 microssegundo). Conte de 1 a 3 minutos em
 * maquinas mais lentas; no Windows clock() tem resolucao de ~1 ms e o teto
 * de 2 s por repeticao passa a pesar mais. A rodada "quick" (N = 1000 e
 * 5000) leva menos de 1 s. O custo dominante sao as remocoes no Vector e na
 * SLL com N grande (cada lote de N/20 remocoes custa O(N) por remocao).
 *
 * USO (executado a partir da raiz do projeto)
 *   build/bench.exe                          -> rodada completa em analysis/
 *   build/bench.exe quick                    -> so N = 1000 e 5000
 *   build/bench.exe freq_hz=3.6e9            -> frequencia manual
 *   build/bench.exe freq_hz=3.6e9 out=analysis
 *   (no Linux: ./build/bench.exe ...)
 *
 *   quick, freq_hz=<Hz> e out=<pasta> podem vir em qualquer ordem; um
 *   argumento desconhecido imprime o uso e termina com erro.
 *
 * O arquivo e compilado sozinho (alvo "bench" do Makefile) com os .c de src/
 * EXCETO src/main.c: dois main() quebrariam o link.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "cpu_detect.h"          /* header-only, esta ao lado deste arquivo  */
#include "DataTypes/dataTypes.h" /* SLL, Vector e AVL                        */
#include "Timestamp/Timestamp.h" /* timestamp_parse, TIMESTAMP_STR_SIZE      */

/* ---------------------------- parametros --------------------------------- */

#define REPS_PER_POINT        10     /* medicoes por ponto                    */
#define TARGET_TICKS          100.0  /* alvo: 100 ticks de clock() acumulados  */
#define MAX_SECONDS_PER_POINT 2.0    /* teto de tempo cronometrado por ponto   */
#define BATCH_DIVISOR         20     /* K = max(1, N/20)                       */
#define RNG_SEED              0x9E3779B97F4A7C15ULL
#define DEFAULT_OUT_DIR       "analysis"
#define MAX_POINTS            16

static const char *const DATA_CANDIDATES[] = {
    "data/timestamps_100k.txt",
    "Data/timestamps_100k.txt"
};

static const int N_FULL[]  = { 1000, 5000, 10000, 50000, 100000 };
static const int N_QUICK[] = { 1000, 5000 };

/* ---------------------------- tipos -------------------------------------- */

typedef enum {
    STRUCT_SLL = 0,
    STRUCT_VECTOR = 1,
    STRUCT_AVL = 2
} StructKind;

typedef enum {
    OP_INSERT = 0,
    OP_REMOVE = 1,
    OP_LIST = 2
} OpKind;

static const char *const STRUCT_NAMES[] = { "sll", "vector", "avl" };
static const char *const OP_NAMES[]     = { "insert", "remove", "list" };

/* Uma estrutura de dados + o buffer usado pela listagem. */
typedef struct {
    StructKind kind;
    SLinkedList *sll;
    Vector *vec;
    AVLTree *avl;
    time_t *list_buffer;
    size_t list_capacity;
    int vector_capacity;
} Container;

/* Resultado de um ponto (um N de uma operacao de uma estrutura). */
typedef struct {
    double median;
    double min;
    double max;
    int zero_ticks;
} PointStats;

/* ---------------------------- utilitarios -------------------------------- */

static void fatal(const char *message)
{
    fprintf(stderr, "erro: %s\n", message);
    exit(EXIT_FAILURE);
}

static size_t bench_max(size_t a, size_t b)
{
    return a > b ? a : b;
}

/* ------------------------ container (API publica) ------------------------ */

static void container_reset(Container *c);

static void container_init(Container *c, StructKind kind, size_t n)
{
    c->kind = kind;
    c->sll = NULL;
    c->vec = NULL;
    c->avl = NULL;
    c->list_capacity = n;
    c->list_buffer = malloc(n * sizeof *c->list_buffer);
    if (c->list_buffer == NULL)
        fatal("malloc falhou (buffer da listagem)");

    /* capacidade suficiente para N elementos: nenhum realloc cronometrado */
    c->vector_capacity = (int)n + 1;

    container_reset(c);
}

static void container_reset(Container *c)
{
    switch (c->kind) {
    case STRUCT_SLL:
        slDestroy(c->sll);
        c->sll = slCreate();
        if (c->sll == NULL)
            fatal("slCreate falhou");
        break;

    case STRUCT_VECTOR:
        vDestroy(c->vec);
        c->vec = vCreate(c->vector_capacity);
        if (c->vec == NULL)
            fatal("vCreate falhou");
        break;

    case STRUCT_AVL:
        avlDestroy(c->avl);
        c->avl = avlCreate();
        if (c->avl == NULL)
            fatal("avlCreate falhou");
        break;
    }
}

static void container_destroy(Container *c)
{
    slDestroy(c->sll);
    vDestroy(c->vec);
    avlDestroy(c->avl);

    free(c->list_buffer);

    c->sll = NULL;
    c->vec = NULL;
    c->avl = NULL;
    c->list_buffer = NULL;
}

static int container_insert(Container *c, time_t value)
{
    switch (c->kind) {
    case STRUCT_SLL:
        return slInsert(c->sll, value);
    case STRUCT_VECTOR:
        return vInsert(c->vec, value);
    case STRUCT_AVL:
        return avlInsert(c->avl, value);
    }

    return 0;
}

static int container_remove(Container *c, time_t value)
{
    switch (c->kind) {
    case STRUCT_SLL:
        return slRemove(c->sll, value);
    case STRUCT_VECTOR:
        return vRemove(c->vec, value);
    case STRUCT_AVL:
        return avlRemove(c->avl, value);
    }

    return 0;
}

/* Lista tudo em ordem cronologica; devolve quantos elementos foram listados. */
static long container_list(Container *c)
{
    switch (c->kind) {
    case STRUCT_SLL:
        return (long)slListAscending(c->sll, c->list_buffer);
    case STRUCT_VECTOR:
        return (long)vListAscending(c->vec, c->list_buffer);
    case STRUCT_AVL:
        return (long)avlListAscending(c->avl, c->list_buffer,
                                      c->list_capacity);
    }

    return 0;
}

static size_t container_size(const Container *c)
{
    switch (c->kind) {
    case STRUCT_SLL:
        return (size_t)c->sll->size;
    case STRUCT_VECTOR:
        return (size_t)c->vec->size;
    case STRUCT_AVL:
        return c->avl->size;
    }

    return 0;
}

/* --------------------------- dados de entrada ---------------------------- */

static time_t *load_timestamps(const char *path, size_t *out_count)
{
    FILE *file = fopen(path, "r");
    time_t *values;
    size_t count = 0;
    size_t capacity = 1024;
    unsigned long long line_number = 0;
    char line[128];

    if (file == NULL)
        return NULL;

    values = malloc(capacity * sizeof *values);
    if (values == NULL)
        fatal("malloc falhou (dados)");

    while (fgets(line, (int)sizeof line, file) != NULL) {
        time_t value;

        line_number++;

        /* o arquivo pode estar em CRLF ou LF; timestamp_parse e estrito */
        line[strcspn(line, "\r\n")] = '\0';

        if (line[0] == '\0')
            continue; /* tolera uma linha vazia no final */

        if (timestamp_parse(line, &value) != 0) {
            fprintf(stderr,
                    "erro: linha %llu de '%s' nao esta no formato "
                    "\"YYYY-MM-DD HH:MM:SS\"\n",
                    line_number, path);
            exit(EXIT_FAILURE);
        }

        if (count == capacity) {
            time_t *grown;

            capacity *= 2;
            grown = realloc(values, capacity * sizeof *grown);
            if (grown == NULL)
                fatal("realloc falhou (dados)");
            values = grown;
        }

        values[count++] = value;
    }

    fclose(file);

    if (count == 0) {
        free(values);
        return NULL;
    }

    *out_count = count;
    return values;
}

static const char *find_data_file(void)
{
    size_t i;

    for (i = 0; i < sizeof DATA_CANDIDATES / sizeof DATA_CANDIDATES[0]; i++) {
        FILE *file = fopen(DATA_CANDIDATES[i], "r");

        if (file != NULL) {
            fclose(file);
            return DATA_CANDIDATES[i];
        }
    }

    return NULL;
}

/* ----------------------------- PRNG portavel ----------------------------- */

static unsigned long long rng_state = RNG_SEED;

static unsigned long long xorshift64(void)
{
    unsigned long long x = rng_state;

    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;

    rng_state = x;
    return x;
}

/* Fisher-Yates: mesma sequencia em qualquer plataforma. */
static void shuffle(time_t *values, size_t count)
{
    size_t i;

    for (i = count; i > 1; i--) {
        size_t j = (size_t)(xorshift64() % (unsigned long long)i);
        time_t tmp = values[i - 1];

        values[i - 1] = values[j];
        values[j] = tmp;
    }
}

/* ----------------------------- medicao ------------------------------------ */

/* Resolucao do clock() em segundos (1 tick), medida na pratica. */
static double clock_resolution(void)
{
    clock_t start = clock();
    clock_t now;

    do {
        now = clock();
    } while (now == start);

    return (double)(now - start) / (double)CLOCKS_PER_SEC;
}

/*
 * Monta a estrutura base do ponto (NAO cronometrado).
 * insert: N-K elementos. remove/list: N elementos.
 */
static void build_base(Container *c, OpKind op, const time_t *data,
                       size_t n, size_t k)
{
    size_t base = (op == OP_INSERT) ? (n - k) : n;
    size_t i;
    int ok = 1;

    container_reset(c);

    for (i = 0; i < base; i++)
        ok &= container_insert(c, data[i]);

    if (!ok || container_size(c) != base)
        fatal("falha ao montar a estrutura base");
}

/*
 * Uma repeticao: devolve os ciclos por operacao.
 * Soma apenas o tempo cronometrado dos lotes; montar/reconstruir a base fica
 * fora do cronometro. *zero_ticks = 1 quando nem 1 tick foi acumulado.
 */
static double measure_rep(Container *c, OpKind op, const time_t *data,
                          size_t n, const time_t *batch, size_t k,
                          double freq_hz, int *zero_ticks)
{
    clock_t timed_ticks = 0;
    long long operations = 0;
    clock_t loop_start = clock();

    *zero_ticks = 0;

    for (;;) {
        clock_t t0;
        clock_t t1;

        build_base(c, op, data, n, k);

        if (op == OP_LIST) {
            long listed;

            t0 = clock();
            listed = container_list(c);
            t1 = clock();

            /* a conferencia fica FORA do trecho cronometrado */
            if ((size_t)listed != n)
                fatal("listagem devolveu uma contagem diferente de n");

            operations += 1;
        } else {
            long long done = 0;
            size_t j;

            /* o teste da operacao fica fora do laco cronometrado */
            t0 = clock();
            if (op == OP_INSERT) {
                for (j = 0; j < k; j++)
                    done += container_insert(c, batch[j]);
            } else {
                for (j = 0; j < k; j++)
                    done += container_remove(c, batch[j]);
            }
            t1 = clock();

            if ((size_t)done != k)
                fatal("insercao/remocao falhou durante a medicao");

            operations += (long long)k;
        }

        timed_ticks += (t1 - t0);

        if ((double)timed_ticks >= TARGET_TICKS)
            break;

        if ((double)timed_ticks / (double)CLOCKS_PER_SEC >=
            MAX_SECONDS_PER_POINT)
            break;

        /* trava de seguranca: se cada rodada custa < 1 tick, o acumulado
         * nunca chega a TARGET_TICKS; o relogio impede o laco infinito. */
        if ((double)(clock() - loop_start) / (double)CLOCKS_PER_SEC >=
            MAX_SECONDS_PER_POINT)
            break;
    }

    if (timed_ticks <= 0) {
        /* limite superior: 1 tick para todas as operacoes do ponto */
        *zero_ticks = 1;
        timed_ticks = 1;
    }

    return ((double)timed_ticks / (double)CLOCKS_PER_SEC) * freq_hz /
           (double)operations;
}

static int compare_double(const void *a, const void *b)
{
    double x = *(const double *)a;
    double y = *(const double *)b;

    return (x > y) - (x < y);
}

/*
 * Um ponto: aquecimento descartado + 10 repeticoes.
 * Mediana = media dos dois valores centrais (5o e 6o de 10).
 */
static PointStats measure_point(Container *c, OpKind op, size_t n,
                                const time_t *data, double freq_hz,
                                time_t *batch, size_t k)
{
    double samples[REPS_PER_POINT];
    PointStats stats;
    int zero = 0;
    int i;

    (void)measure_rep(c, op, data, n, batch, k, freq_hz, &zero); /* aquecimento */

    stats.zero_ticks = 0;

    for (i = 0; i < REPS_PER_POINT; i++) {
        samples[i] = measure_rep(c, op, data, n, batch, k, freq_hz, &zero);

        if (zero)
            stats.zero_ticks = 1;
    }

    qsort(samples, REPS_PER_POINT, sizeof samples[0], compare_double);

    stats.median = (samples[REPS_PER_POINT / 2 - 1] +
                    samples[REPS_PER_POINT / 2]) / 2.0;
    stats.min = samples[0];
    stats.max = samples[REPS_PER_POINT - 1];

    /* o graphs.py usa log: nunca gravar 0 (com %.1f, 1e-9 tambem sairia
     * como "0.0"; 0.1 e o menor valor representavel na saida) */
    if (!(stats.median >= 0.1))
        stats.median = 0.1;
    if (!(stats.min >= 0.1))
        stats.min = 0.1;
    if (!(stats.max >= 0.1))
        stats.max = 0.1;

    return stats;
}

/* ------------------------------ saida ------------------------------------ */

static void print_usage(const char *program)
{
    fprintf(stderr,
            "uso: %s [quick] [freq_hz=<Hz>] [out=<pasta>]\n"
            "  quick         usa apenas N = 1000 e 5000 (teste rapido)\n"
            "  freq_hz=<Hz>  sobrescreve a frequencia detectada "
            "(ex.: freq_hz=3.6e9)\n"
            "  out=<pasta>   pasta dos 9 arquivos de resultado "
            "(padrao: analysis)\n",
            program);
}

/* Remove barras sobrando no fim ("analysis/" -> "analysis"). */
static void trim_trailing_slashes(char *path)
{
    size_t len = strlen(path);

    while (len > 1 && path[len - 1] == '/') {
        path[len - 1] = '\0';
        len--;
    }
}

/* ------------------------------- main ------------------------------------ */

int main(int argc, char **argv)
{
    int quick = 0;
    double freq_override = 0.0;
    char out_dir[512] = DEFAULT_OUT_DIR;
    const char *data_path;
    time_t *data;
    size_t total = 0;
    char cpu_model[256];
    double freq_hz = 0.0;
    double resolution;
    const char *os_name;
    const int *n_values;
    int n_count;
    size_t max_n;
    time_t *batch;
    PointStats results[MAX_POINTS];
    int arg;
    int s;
    int o;
    int idx;

    /* Nenhuma saida no stdout: as listagens nao devem medir o terminal. */
    {
        FILE *null_stream;

#ifdef _WIN32
        null_stream = freopen("NUL", "w", stdout);
#else
        null_stream = freopen("/dev/null", "w", stdout);
#endif

        if (null_stream == NULL)
            fprintf(stderr, "aviso: nao consegui redirecionar o stdout\n");
    }

    /* ---------------- argumentos (em qualquer posicao) ---------------- */

    for (arg = 1; arg < argc; arg++) {
        if (strcmp(argv[arg], "quick") == 0) {
            quick = 1;
        } else if (strncmp(argv[arg], "freq_hz=", 8) == 0) {
            const char *text = argv[arg] + 8;
            char *end = NULL;
            double value = strtod(text, &end);

            if (end == text || end == NULL || *end != '\0' || value <= 0.0) {
                fprintf(stderr, "erro: freq_hz invalido: '%s'\n", argv[arg]);
                print_usage(argv[0]);
                return EXIT_FAILURE;
            }

            freq_override = value;
        } else if (strncmp(argv[arg], "out=", 4) == 0) {
            const char *value = argv[arg] + 4;

            if (*value == '\0') {
                fprintf(stderr, "erro: out= sem pasta\n");
                print_usage(argv[0]);
                return EXIT_FAILURE;
            }

            if (strlen(value) >= sizeof out_dir) {
                fprintf(stderr, "erro: out= com caminho longo demais\n");
                return EXIT_FAILURE;
            }

            strcpy(out_dir, value);
        } else {
            fprintf(stderr, "erro: argumento desconhecido: '%s'\n", argv[arg]);
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    trim_trailing_slashes(out_dir);

    /* ------------------------- CPU e frequencia ----------------------- */

    if (detect_cpu(cpu_model, sizeof cpu_model, &freq_hz) == 0)
        freq_hz = 0.0;

    if (freq_override > 0.0)
        freq_hz = freq_override;

    if (freq_hz <= 0.0) {
        fprintf(stderr,
                "Nao consegui detectar a frequencia. "
                "Rode com freq_hz=<Hz>.\n");
        return EXIT_FAILURE;
    }

    fprintf(stderr, "CPU: %s | %.0f MHz\n", cpu_model, freq_hz / 1e6);

#if defined(_WIN32)
    os_name = "windows";
#elif defined(__linux__)
    os_name = "linux";
#else
    os_name = "outro";
#endif

    resolution = clock_resolution();

    /* ------------------------------ dados ----------------------------- */

    data_path = find_data_file();
    if (data_path == NULL)
        fatal("nao encontrei data/timestamps_100k.txt nem "
              "Data/timestamps_100k.txt (rode a partir da raiz do projeto)");

    data = load_timestamps(data_path, &total);
    if (data == NULL)
        fatal("nao consegui ler os timestamps");

    n_values = quick ? N_QUICK : N_FULL;
    n_count = quick ? (int)(sizeof N_QUICK / sizeof N_QUICK[0])
                    : (int)(sizeof N_FULL / sizeof N_FULL[0]);

    if (n_count > MAX_POINTS)
        fatal("MAX_POINTS pequeno demais");

    max_n = (size_t)n_values[n_count - 1];

    if (total < max_n) {
        fprintf(stderr,
                "erro: '%s' tem %llu timestamps, preciso de %llu\n",
                data_path, (unsigned long long)total,
                (unsigned long long)max_n);
        return EXIT_FAILURE;
    }

    shuffle(data, total);

    fprintf(stderr, "dados: %s (%llu timestamps embaralhados)\n",
            data_path, (unsigned long long)total);

    batch = malloc((max_n / BATCH_DIVISOR + 1) * sizeof *batch);
    if (batch == NULL)
        fatal("malloc falhou (lote)");

    /* --------------------------- 9 arquivos --------------------------- */

    for (s = 0; s < 3; s++) {
        for (o = 0; o < 3; o++) {
            char path[1024];
            FILE *out;
            int file_index = s * 3 + o;
            int any_zero = 0;

            fprintf(stderr, "[%d/9] %s %s\n", file_index + 1,
                    STRUCT_NAMES[s], OP_NAMES[o]);

            /* abre o arquivo ANTES de medir: se a pasta nao existir,
             * aborta imediatamente em vez de perder a rodada inteira */
            snprintf(path, sizeof path, "%s/%s_%s.txt", out_dir,
                     STRUCT_NAMES[s], OP_NAMES[o]);

            out = fopen(path, "w");
            if (out == NULL) {
                fprintf(stderr,
                        "erro: nao consegui criar '%s' "
                        "(a pasta '%s' existe?)\n", path, out_dir);
                free(batch);
                free(data);
                return EXIT_FAILURE;
            }

            fprintf(out, "# estrutura=%s operacao=%s\n",
                    STRUCT_NAMES[s], OP_NAMES[o]);
            fprintf(out, "# cpu=%s freq_base_hz=%.0f os=%s\n",
                    cpu_model, freq_hz, os_name);
            fprintf(out,
                    "# clock_resolution_s=%.9g repeticoes=%d "
                    "metrica=mediana_de_ciclos_por_operacao\n",
                    resolution, REPS_PER_POINT);
            fflush(out);

            for (idx = 0; idx < n_count; idx++) {
                size_t n = (size_t)n_values[idx];
                size_t k = bench_max(1, n / BATCH_DIVISOR);
                Container container;
                int j;

                if (o == OP_INSERT) {
                    /* lote = os K ultimos elementos (ainda nao inseridos) */
                    for (j = 0; j < (int)k; j++)
                        batch[j] = data[n - k + (size_t)j];
                } else if (o == OP_REMOVE) {
                    /* K elementos espalhados pela ordem de insercao:
                     * posicoes arbitrarias -> caso medio */
                    for (j = 0; j < (int)k; j++)
                        batch[j] = data[((size_t)j * n) / k];
                }

                container_init(&container, (StructKind)s, n);

                results[idx] = measure_point(&container, (OpKind)o, n, data,
                                             freq_hz, batch, k);

                container_destroy(&container);

                if (results[idx].zero_ticks)
                    any_zero = 1;

                fprintf(stderr,
                        "  n=%d mediana=%.1f min=%.1f max=%.1f ciclos/op\n",
                        n_values[idx], results[idx].median,
                        results[idx].min, results[idx].max);
            }

            /* cabecalho final + dados (o AVISO so pode ser decidido aqui) */
            if (any_zero)
                fprintf(out,
                        "# AVISO: algum ponto acumulou 0 ticks; foi gravado "
                        "1 tick/operacoes (limite superior)\n");

            fprintf(out, "# n,mediana_ciclos,min_ciclos,max_ciclos\n");
            fflush(out);

            for (idx = 0; idx < n_count; idx++) {
                fprintf(out, "%d,%.1f,%.1f,%.1f\n", n_values[idx],
                        results[idx].median, results[idx].min,
                        results[idx].max);
                fflush(out);
            }

            fclose(out);
        }
    }

    free(batch);
    free(data);

    fprintf(stderr, "ok: 9 arquivos escritos em '%s/'\n", out_dir);

    return EXIT_SUCCESS;
}
