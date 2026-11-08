#include <stdio.h>
#include <string.h>
#include <time.h>

#include "Timestamp/Timestamp.h"

#ifdef _WIN32
#define timegm _mkgmtime
#endif

static int is_leap(int y)
{
    return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
}

static int days_in_month(int y, int m)
{
    static const int dm[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    return (m == 2 && is_leap(y)) ? 29 : dm[m - 1];
}

int timestamp_parse(const char *str, time_t *out)
{
    int y, mo, d, h, mi, s;
    struct tm tm = { 0 };

    if (str == NULL || out == NULL)
        return -1;

    if (strlen(str) != 19)
        return -1;

    if (str[4] != '-' || str[7] != '-' || str[10] != ' ' ||
        str[13] != ':' || str[16] != ':')
        return -1;

    if (sscanf(str, "%4d-%2d-%2d %2d:%2d:%2d",
               &y, &mo, &d, &h, &mi, &s) != 6)
        return -1;

    if (mo < 1 || mo > 12)
        return -1;
    if (d < 1 || d > days_in_month(y, mo))
        return -1;
    if (h > 23 || mi > 59 || s > 59)
        return -1;

    tm.tm_year = y - 1900;
    tm.tm_mon = mo - 1;
    tm.tm_mday = d;
    tm.tm_hour = h;
    tm.tm_min = mi;
    tm.tm_sec = s;
    tm.tm_isdst = 0;

    *out = timegm(&tm);

    return 0;
}

int timestamp_compare(time_t a, time_t b)
{
    return (a > b) - (a < b);
}

void timestamp_to_string(time_t t, char *buf)
{
    struct tm *tm = gmtime(&t);

    if (tm == NULL) {
        buf[0] = '\0';
        return;
    }

    strftime(buf, TIMESTAMP_STR_SIZE, "%Y-%m-%d %H:%M:%S", tm);
}
