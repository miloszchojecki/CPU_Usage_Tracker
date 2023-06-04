#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"

void readCpuStats(CpuStats *stats, int core) {
    FILE *file;
    char line[MAX_LINE_LENGTH];
    char cpuLabel[MAX_LABEL_LENGTH];

    sprintf(cpuLabel, "cpu%d", core);

    file = fopen("/proc/stat", "r");
    if (file == NULL) {
        perror("Failed to open /proc/stat");
        exit(1);
    }

    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, cpuLabel, strlen(cpuLabel)) == 0) {
            sscanf(line + strlen(cpuLabel), "%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
                   &stats->user, &stats->nice, &stats->system, &stats->idle,
                   &stats->iowait, &stats->irq, &stats->softirq, &stats->steal,
                   &stats->guest, &stats->guest_nice);
            break;
        }
    }
    fclose(file);
}
