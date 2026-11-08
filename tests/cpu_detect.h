/*
 * cpu_detect.h - detecta modelo e frequência base (nominal) da CPU em tempo de execução.
 *
 *   int detect_cpu(char *model, size_t model_size, double *freq_hz);
 *     Retorna 1 se achou a frequência (em *freq_hz, Hz), 0 caso contrário.
 *     O modelo é sempre preenchido ("desconhecido" se não achar).
 *
 * Windows: registro (HKLM\HARDWARE\DESCRIPTION\System\CentralProcessor\0: ~MHz e ProcessorNameString).
 * Linux:   /proc/cpuinfo (modelo) + /sys/.../cpufreq/base_frequency (intel_pstate)
 *          ou o "@ x.xxGHz" do nome do modelo (CPUs Intel). AMD/VM/WSL costumam cair
 *          no caso "não achou": nesse caso passe o valor manualmente.
 * NÃO usa a frequência atual (turbo/carga): ela muda entre execuções.
 */
#ifndef CPU_DETECT_H
#define CPU_DETECT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX                 /* evita que o windows.h defina min/max */
#include <windows.h>

/* Windows: lê modelo e frequência nominal da CPU no registro. Retorna 1 se achou a frequência. */
static int detect_cpu(char *model, size_t model_size, double *freq_hz)
{
    HKEY key;
    DWORD mhz = 0, mhz_size = sizeof mhz, type = 0;
    char name[128];
    DWORD name_size = sizeof name - 1;
    int found = 0;

    snprintf(model, model_size, "desconhecido");

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                      "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                      0, KEY_READ, &key) != ERROR_SUCCESS)
        return 0;

    if (RegQueryValueExA(key, "~MHz", NULL, &type, (LPBYTE)&mhz, &mhz_size) == ERROR_SUCCESS
        && type == REG_DWORD && mhz > 0) {
        *freq_hz = (double)mhz * 1e6;
        found = 1;
    }

    if (RegQueryValueExA(key, "ProcessorNameString", NULL, NULL,
                         (LPBYTE)name, &name_size) == ERROR_SUCCESS) {
        name[name_size] = '\0';
        snprintf(model, model_size, "%s", name + strspn(name, " "));
    }

    RegCloseKey(key);
    return found;
}

#else  /* Linux e outros POSIX */

/* "Intel(R) Core(TM) i7-9750H CPU @ 2.60GHz" -> 2.6e9. Devolve 0 se não houver "@ <n>GHz/MHz". */
static double freq_from_model_name(const char *name)
{
    const char *at = strchr(name, '@');
    double v = 0;
    char unit = 0;

    if (at == NULL || sscanf(at + 1, " %lf %c", &v, &unit) != 2 || v <= 0)
        return 0;
    if (unit == 'G' || unit == 'g')
        return v * 1e9;
    if (unit == 'M' || unit == 'm')
        return v * 1e6;
    return 0;
}

/* Linux: modelo via /proc/cpuinfo; frequência base via sysfs (intel_pstate) ou pelo "@ x.xxGHz" do nome. */
static int detect_cpu(char *model, size_t model_size, double *freq_hz)
{
    FILE *f;
    char line[256];
    double khz = 0;

    snprintf(model, model_size, "desconhecido");
    *freq_hz = 0;

    f = fopen("/proc/cpuinfo", "r");
    if (f) {
        while (fgets(line, sizeof line, f)) {
            if (strncmp(line, "model name", 10) == 0) {
                const char *p = strchr(line, ':');

                if (p) {
                    p++;
                    p += strspn(p, " \t");
                    snprintf(model, model_size, "%s", p);
                    model[strcspn(model, "\r\n")] = '\0';
                }
                break;
            }
        }
        fclose(f);
    }

    /* 1) frequência base declarada pelo driver, em kHz (existe com intel_pstate) */
    f = fopen("/sys/devices/system/cpu/cpu0/cpufreq/base_frequency", "r");
    if (f) {
        int ok = (fscanf(f, "%lf", &khz) == 1 && khz > 0);

        fclose(f);
        if (ok) {
            *freq_hz = khz * 1e3;
            return 1;
        }
    }

    /* 2) "@ 2.60GHz" no nome do modelo (CPUs Intel) */
    *freq_hz = freq_from_model_name(model);
    return *freq_hz > 0;
}
#endif

#endif /* CPU_DETECT_H */