#include <stdio.h>
#include <string.h>
#include <time.h>

#include "Timestamp/Timestamp.h"

#ifndef TIMESTAMP_STR_SIZE
#define TIMESTAMP_STR_SIZE 20 /* "YYYY-MM-DD HH:MM:SS" + '\0' */
#endif

/*
 * Pure arithmetic on the proleptic Gregorian calendar (Howard Hinnant's
 * days_from_civil / civil_from_days). It does not use mktime, timegm,
 * _mkgmtime or localtime, so the result is always UTC, supports any year
 * 0000-9999, and is identical on Linux and Windows.
 */

static long long days_from_civil(int y, int m, int d)
{
    long long yy = y - (m <= 2);
    long long era = (yy >= 0 ? yy : yy - 399) / 400;
    long long yoe = yy - era * 400;
    long long doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
    long long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;

    return era * 146097 + doe - 719468;
}

static void civil_from_days(long long z, int *y, int *m, int *d)
{
    long long era, doe, yoe, yy, doy, mp;

    z += 719468;
    era = (z >= 0 ? z : z - 146096) / 146097;
    doe = z - era * 146097;
    yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    yy = yoe + era * 400;
    doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    mp = (5 * doy + 2) / 153;
    *d = (int)(doy - (153 * mp + 2) / 5 + 1);
    *m = (int)(mp < 10 ? mp + 3 : mp - 9);
    *y = (int)(yy + (*m <= 2));
}

static int is_leap(int y)
{
    return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
}

static int days_in_month(int y, int m)
{
    static const int dm[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    return (m == 2 && is_leap(y)) ? 29 : dm[m - 1];
}

static int all_digits(const char *s, int n)
{
    int i;

    for (i = 0; i < n; i++)
        if (s[i] < '0' || s[i] > '9')
            return 0;
    return 1;
}

static int to_int(const char *s, int n)
{
    int v = 0, i;

    for (i = 0; i < n; i++)
        v = v * 10 + (s[i] - '0');
    return v;
}

int timestamp_parse(const char *str, time_t *out)
{
    int y, mo, d, h, mi, s;
    long long v;

    if (!str || !out)
        return -1;

    /* strict layout: "YYYY-MM-DD HH:MM:SS", exactly 19 characters */
    if (strlen(str) != 19)
        return -1;
    if (str[4] != '-' || str[7] != '-' || str[10] != ' ' ||
        str[13] != ':' || str[16] != ':')
        return -1;
    if (!all_digits(str, 4) || !all_digits(str + 5, 2) ||
        !all_digits(str + 8, 2) || !all_digits(str + 11, 2) ||
        !all_digits(str + 14, 2) || !all_digits(str + 17, 2))
        return -1;

    y = to_int(str, 4);
    mo = to_int(str + 5, 2);
    d = to_int(str + 8, 2);
    h = to_int(str + 11, 2);
    mi = to_int(str + 14, 2);
    s = to_int(str + 17, 2);

    if (mo < 1 || mo > 12)
        return -1;
    if (d < 1 || d > days_in_month(y, mo))
        return -1;
    if (h > 23 || mi > 59 || s > 59) /* leap second (60) is rejected */
        return -1;

    v = days_from_civil(y, mo, d) * 86400LL + h * 3600LL + mi * 60LL + s;

    if ((long long)(time_t)v != v) /* does not fit a 32-bit time_t */
        return -1;

    *out = (time_t)v;
    return 0;
}

int timestamp_compare(time_t a, time_t b)
{
    return (a > b) - (a < b); /* never a - b: it can overflow int */
}

void timestamp_to_string(time_t t, char *buf)
{
    long long v = (long long)t;
    long long days = v / 86400;
    long long rem = v % 86400;
    unsigned hh, mm, ss;
    int y, m, d;

    if (rem < 0) { /* floor division for instants before 1970 */
        rem += 86400;
        days -= 1;
    }
    civil_from_days(days, &y, &m, &d);

    hh = (unsigned)(rem / 3600) % 24u;
    mm = (unsigned)((rem % 3600) / 60) % 60u;
    ss = (unsigned)(rem % 60) % 60u;

    /* snprintf never writes more than TIMESTAMP_STR_SIZE bytes */
    snprintf(buf, TIMESTAMP_STR_SIZE, "%04d-%02d-%02d %02u:%02u:%02u",
             y, m, d, hh, mm, ss);
}