#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_CORES 8
#define MAX_LINE_LENGTH 256
#define MAX_LABEL_LENGTH 8
#define clear() printf("\033[H\033[J")

typedef struct {
    unsigned long user;
    unsigned long nice;
    unsigned long system;
    unsigned long idle;
    unsigned long iowait;
    unsigned long irq;
    unsigned long softirq;
    unsigned long steal;
    unsigned long guest;
    unsigned long guest_nice;
} CpuStats;

static void readCpuStats(CpuStats *stats, int core) {
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

int main() {
    while(1) {
        sleep(1);
        clear();
        CpuStats prevStats[MAX_CORES], currStats[MAX_CORES];

        for (int i = 0; i < MAX_CORES; i++) {
            readCpuStats(&prevStats[i], i);
        }

        sleep(1);

        for (int i = 0; i < MAX_CORES; i++) {
            readCpuStats(&currStats[i], i);
        }

        unsigned long prevTotal = 0, currTotal = 0;
        unsigned long prevIdle = 0, currIdle = 0;

        for (int i = 0; i < MAX_CORES; i++) {
            prevTotal += prevStats[i].user + prevStats[i].nice + prevStats[i].system +
                         prevStats[i].idle + prevStats[i].iowait + prevStats[i].irq +
                         prevStats[i].softirq + prevStats[i].steal;

            currTotal += currStats[i].user + currStats[i].nice + currStats[i].system +
                         currStats[i].idle + currStats[i].iowait + currStats[i].irq +
                         currStats[i].softirq + currStats[i].steal;

            prevIdle += prevStats[i].idle + prevStats[i].iowait;
            currIdle += currStats[i].idle + currStats[i].iowait;
        }

        unsigned long prevTotalDiff = currTotal - prevTotal;
        unsigned long currIdleDiff = currIdle - prevIdle;

        double cpuUsage = (double) (prevTotalDiff - currIdleDiff) / (double) prevTotalDiff * 100.0;

        printf("Overall CPU Usage: %.2f%%\n", cpuUsage);

        for (int i = 0; i < MAX_CORES; i++) {
            prevTotal = prevStats[i].user + prevStats[i].nice + prevStats[i].system +
                        prevStats[i].idle + prevStats[i].iowait + prevStats[i].irq +
                        prevStats[i].softirq + prevStats[i].steal;

            currTotal = currStats[i].user + currStats[i].nice + currStats[i].system +
                        currStats[i].idle + currStats[i].iowait + currStats[i].irq +
                        currStats[i].softirq + currStats[i].steal;

            prevIdle = prevStats[i].idle + prevStats[i].iowait;
            currIdle = currStats[i].idle + currStats[i].iowait;

            prevTotalDiff = currTotal - prevTotal;
            currIdleDiff = currIdle - prevIdle;

            double coreUsage = (double) (prevTotalDiff - currIdleDiff) / (double) prevTotalDiff * 100.0;

            printf("Core %d Usage: %.2f%%\n", i, coreUsage);
        }
    }
}
