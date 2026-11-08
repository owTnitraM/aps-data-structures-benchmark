#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <time.h>

/* "YYYY-MM-DD HH:MM:SS" + '\0' */
#define TIMESTAMP_STR_SIZE 20

/* Retorna 0 em caso de sucesso e -1 se a string for invalida. */
int timestamp_parse(const char *str, time_t *out);

int timestamp_compare(time_t a, time_t b);

/* Escreve "YYYY-MM-DD HH:MM:SS" (UTC) em buf. */
void timestamp_to_string(time_t t, char *buf);

#endif
