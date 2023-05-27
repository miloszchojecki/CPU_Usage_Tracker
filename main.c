#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_LINE_LENGTH 256
#define MAX_LABEL_LENGTH 8
//#define clear() printf("\033[H\033[J")

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

typedef struct {
    CpuStats *prevStats;
    CpuStats *currStats;
    long numOfCores;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    double *usage;
} ThreadData;

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

static void *readerThread(void *arg) {
    ThreadData *data = (ThreadData *) arg;
    CpuStats *prevStats = data->prevStats;
    CpuStats *currStats = data->currStats;
    long numOfCores = data->numOfCores;

    pthread_mutex_lock(&(data->mutex));
    for (short i = 0; i < numOfCores; i++) {
        readCpuStats(&prevStats[i], i);
    }

    sleep(1);

    for (short i = 0; i < numOfCores; i++) {
        readCpuStats(&currStats[i], i);
    }
    pthread_cond_signal(&(data->cond));
    pthread_mutex_unlock(&(data->mutex));

    sleep(1);

    pthread_exit(NULL);
}

static void *analyzerThread(void *arg) {
    ThreadData *data = (ThreadData *) arg;
    CpuStats *prevStats = data->prevStats;
    CpuStats *currStats = data->currStats;
    long numOfCores = data->numOfCores;
    double *usage = data->usage;

    pthread_mutex_lock(&(data->mutex));
    unsigned long prevTotal = 0, currTotal = 0;
    unsigned long prevIdle = 0, currIdle = 0;

    for (short i = 0; i < numOfCores; i++) {
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

    usage[0] = cpuUsage;

    for (short i = 0; i < numOfCores; i++) {
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

        usage[i + 1] = coreUsage;
    }

    pthread_cond_signal(&(data->cond));
    pthread_mutex_unlock(&(data->mutex));

    sleep(1);

    pthread_exit(NULL);
}

static void *printerThread(void *arg) {
    ThreadData *data = (ThreadData *) arg;
    long numOfCores = data->numOfCores;
    double *usage = data->usage;
    pthread_mutex_lock(&(data->mutex));
    printf("Overall CPU Usage: %.2f%%\n", usage[0]);
    for (short i = 0; i < numOfCores; i++) {
        printf("Core %d Usage: %.2f%%\n", i, usage[i + 1]);
    }
    pthread_cond_signal(&(data->cond));
    pthread_mutex_unlock(&(data->mutex));

    pthread_exit(NULL);
}

int main(void) {
    long numOfCores = sysconf(_SC_NPROCESSORS_ONLN);

    ThreadData data;
    data.numOfCores = numOfCores;
    data.prevStats = malloc(sizeof(CpuStats) * (unsigned long) numOfCores);
    data.currStats = malloc(sizeof(CpuStats) * (unsigned long) numOfCores);
    data.usage = malloc(sizeof(double) * (unsigned long) (numOfCores + 1));

    pthread_mutex_init(&(data.mutex), NULL);
    pthread_cond_init(&(data.cond), NULL);

    pthread_t readerThreadId, analyzerThreadId, printerThreadId;

    pthread_create(&readerThreadId, NULL, readerThread, &data);
    pthread_create(&analyzerThreadId, NULL, analyzerThread, &data);
    pthread_create(&printerThreadId, NULL, printerThread, &data);

    pthread_join(readerThreadId, NULL);
    pthread_join(analyzerThreadId, NULL);
    pthread_join(printerThreadId, NULL);

    free(data.prevStats);
    free(data.currStats);
    free(data.usage);

    return 0;
}
