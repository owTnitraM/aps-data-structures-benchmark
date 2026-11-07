#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <time.h>

/* "YYYY-MM-DD HH:MM:SS" (19 characters) + '\0' */
#define TIMESTAMP_STR_SIZE 20

/*
 * Parses a string in the format "YYYY-MM-DD HH:MM:SS" into a time_t
 * (seconds since 1970-01-01 00:00:00 UTC).
 * Returns 0 on success and -1 on error (invalid or malformed string).
 * The result is written to *out. A status code is used instead of a
 * sentinel value because (time_t)-1 is a valid instant (1969-12-31 23:59:59).
 * Behaves the same on Linux and Windows.
 */
int timestamp_parse(const char *str, time_t *out);

/*
 * Compares two timestamps.
 * Returns <0 if a < b, 0 if a == b and >0 if a > b. O(1).
 */
int timestamp_compare(time_t a, time_t b);

/*
 * Writes the timestamp as "YYYY-MM-DD HH:MM:SS" (UTC) into buf.
 * buf must have at least TIMESTAMP_STR_SIZE bytes.
 */
void timestamp_to_string(time_t t, char *buf);

#endif