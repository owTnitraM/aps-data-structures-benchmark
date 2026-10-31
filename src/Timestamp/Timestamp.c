#define _GNU_SOURCE   /* exposes timegm() in glibc (Linux) */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "Timestamp.h"

#ifdef _WIN32
/* Windows (MinGW/MSVC) has no timegm(); _mkgmtime() does the same thing */
#define timegm _mkgmtime
#endif

/*
 * Strict shape check for "YYYY-MM-DD HH:MM:SS": 'd' = one digit, anything
 * else must match literally. Only whitespace (e.g. '\n', '\r') may follow.
 * Same result on every platform, which sscanf() alone does not guarantee.
 */
static int has_valid_shape(const char *s) {
    const char *pattern = "dddd-dd-dd dd:dd:dd";
    int i;

    for (i = 0; pattern[i] != '\0'; i++) {
        if (s[i] == '\0') return 0;
        if (pattern[i] == 'd') {
            if (!isdigit((unsigned char)s[i])) return 0;
        } else if (s[i] != pattern[i]) {
            return 0;
        }
    }
    for (s += i; *s != '\0'; s++) {
        if (!isspace((unsigned char)*s)) return 0;
    }
    return 1;
}

int timestamp_parse(const char *str, time_t *out) {
    int year, month, day, hour, min, sec;
    struct tm t;
    time_t result;

    if (!has_valid_shape(str)) return -1;

    /* (1) string -> struct tm, using sscanf() (works on Linux and Windows) */
    if (sscanf(str, "%d-%d-%d %d:%d:%d",
               &year, &month, &day, &hour, &min, &sec) != 6) return -1;

    memset(&t, 0, sizeof t);
    t.tm_year = year - 1900;   /* years since 1900 */
    t.tm_mon  = month - 1;     /* 0..11 */
    t.tm_mday = day;
    t.tm_hour = hour;
    t.tm_min  = min;
    t.tm_sec  = sec;

    /* (2) struct tm -> time_t in UTC (ignores the local timezone) */
    result = timegm(&t);
    if (result == (time_t)-1 && t.tm_year != 69) return -1;  /* 69 = 1969 */

    *out = result;
    return 0;
}

/* (3) comparison done directly on time_t values: O(1) */
int timestamp_compare(time_t a, time_t b) {
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

void timestamp_to_string(time_t t, char *buf) {
    struct tm *tm_utc = gmtime(&t);
    strftime(buf, TIMESTAMP_STR_SIZE, TIMESTAMP_FORMAT, tm_utc);
}