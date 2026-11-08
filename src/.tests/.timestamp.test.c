/* Testes das funcoes de timestamp. */

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "Timestamp/Timestamp.h"

static int failures = 0;

static void check(const char *name, int ok)
{
    printf("[%s] %s\n", ok ? " ok " : "FAIL", name);
    if (!ok)
        failures++;
}

/* parse succeeds and gives exactly `expected` */
static int parses_to(const char *s, long long expected)
{
    time_t t = 12345;

    return timestamp_parse(s, &t) == 0 && (long long)t == expected;
}

/* parse reports an error */
static int rejects(const char *s)
{
    time_t t = 0;

    return timestamp_parse(s, &t) == -1;
}

/* to_string gives exactly `expected` */
static int formats_to(long long v, const char *expected)
{
    char buf[TIMESTAMP_STR_SIZE];

    timestamp_to_string((time_t)v, buf);
    return strcmp(buf, expected) == 0;
}

int main(void)
{
    time_t a = 0, b = 0, back = 0;
    char buf[TIMESTAMP_STR_SIZE];

    puts("--- timestamp_parse ---");
    check("1970-01-01 00:00:00 -> 0", parses_to("1970-01-01 00:00:00", 0));
    check("1969-12-31 23:59:59 -> -1 (success, not error)", parses_to("1969-12-31 23:59:59", -1));
    check("2000-01-01 00:00:00 -> 946684800", parses_to("2000-01-01 00:00:00", 946684800));
    check("2024-02-29 12:00:00 -> 1709208000 (leap day)", parses_to("2024-02-29 12:00:00", 1709208000));
    check("2038-01-19 03:14:07 -> 2147483647", parses_to("2038-01-19 03:14:07", 2147483647));
    check("rejects 2023-02-29 (not a leap year)", rejects("2023-02-29 00:00:00"));
    check("rejects month 13", rejects("2024-13-01 00:00:00"));
    check("rejects hour 24", rejects("2024-01-01 24:00:00"));
    check("rejects missing time", rejects("2024-01-01"));
    check("rejects wrong separator", rejects("2024/01/01 00:00:00"));
    check("rejects empty string", rejects(""));
    check("rejects NULL", timestamp_parse(NULL, &a) == -1);

    puts("\n--- timestamp_compare ---");
    timestamp_parse("2024-01-01 10:00:00", &a);
    timestamp_parse("2024-01-01 10:00:01", &b);
    check("earlier < later", timestamp_compare(a, b) < 0);
    check("later > earlier", timestamp_compare(b, a) > 0);
    check("equal == 0", timestamp_compare(a, a) == 0);
    check("no int overflow (2^32 vs 0)", sizeof(time_t) < 8 ||
          timestamp_compare((time_t)4294967296LL, 0) > 0);

    puts("\n--- timestamp_to_string ---");
    check("0 -> 1970-01-01 00:00:00", formats_to(0, "1970-01-01 00:00:00"));
    check("-1 -> 1969-12-31 23:59:59", formats_to(-1, "1969-12-31 23:59:59"));
    check("946684800 -> 2000-01-01 00:00:00", formats_to(946684800, "2000-01-01 00:00:00"));
    check("1709208000 -> 2024-02-29 12:00:00", formats_to(1709208000, "2024-02-29 12:00:00"));

    puts("\n--- round trip ---");
    timestamp_to_string(a, buf);
    check("parse -> to_string gives the same text", strcmp(buf, "2024-01-01 10:00:00") == 0);
    check("to_string -> parse gives the same value",
          timestamp_parse(buf, &back) == 0 && back == a);

    printf("\n%s (%d failed)\n", failures ? "SOME CHECKS FAILED" : "ALL CHECKS PASSED", failures);
    return failures ? 1 : 0;
}