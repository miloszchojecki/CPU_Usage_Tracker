#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <signal.h>
#include "main.h"

#define clear() printf("\033[H\033[J")

static _Atomic bool
isRunning = true;

typedef struct {
    CpuStats *prevStats;
    CpuStats *currStats;
    long numOfCores;
    pthread_mutex_t mutex;
    double *usage;
} ThreadData;

typedef struct {
    ThreadData *data;
    FILE *logFile;
    pthread_mutex_t mutex;
} LoggerThreadData;

static void *readerThread(void *arg) {
    ThreadData *data = (ThreadData *) arg;
    CpuStats *currStats = data->currStats;
    long numOfCores = data->numOfCores;
    pthread_mutex_lock(&(data->mutex));
    for (short i = 0; i < numOfCores; i++) {
        readCpuStats(&currStats[i], i);
    }
    pthread_mutex_unlock(&(data->mutex));
    sleep(1);

    while (isRunning) {
        pthread_mutex_lock(&(data->mutex));
        memcpy(data->prevStats, data->currStats, sizeof(CpuStats) * (unsigned long) (numOfCores + 1));
        for (short i = 0; i < numOfCores; i++) {
            readCpuStats(&currStats[i], i);
        }
        pthread_mutex_unlock(&(data->mutex));

        sleep(1);
    }
    pthread_exit(NULL);
}

static void *analyzerThread(void *arg) {
    ThreadData *data = (ThreadData *) arg;
    while (isRunning) {
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

        if (prevTotalDiff == 0) {
            double cpuUsage = 0;
            usage[0] = cpuUsage;
        } else {
            double cpuUsage = (double) (prevTotalDiff - currIdleDiff) / (double) prevTotalDiff * 100.0;
            usage[0] = cpuUsage;
        }

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

            if (prevTotalDiff == 0) {
                double coreUsage = 0;
                usage[i + 1] = coreUsage;
            } else {
                double coreUsage = (double) (prevTotalDiff - currIdleDiff) / (double) prevTotalDiff * 100.0;
                usage[i + 1] = coreUsage;
            }
        }
        pthread_mutex_unlock(&(data->mutex));

        sleep(1);
    }
    pthread_exit(NULL);
}

static void *printerThread(void *arg) {
    ThreadData *data = (ThreadData *) arg;
    while (isRunning) {
        long numOfCores = data->numOfCores;
        double *usage = data->usage;
        pthread_mutex_lock(&(data->mutex));
        clear();
        printf("Overall CPU Usage: %.2f%%\n", usage[0]);
        for (short i = 0; i < numOfCores; i++) {
            printf("Core %d Usage: %.2f%%\n", i, usage[i + 1]);
        }
        pthread_mutex_unlock(&(data->mutex));
        sleep(1);
    }
    pthread_exit(NULL);
}

static void *watchdogThread(void *arg) {
    ThreadData *data = (ThreadData *) arg;
    while (isRunning) {
        sleep(2);

        pthread_mutex_lock(&(data->mutex));
        CpuStats *prevStats = data->prevStats;
        CpuStats *currStats = data->currStats;
        long numOfCores = data->numOfCores;
        pthread_mutex_unlock(&(data->mutex));

        bool threadsRunning = false;
        for (short i = 0; i < numOfCores; i++) {
            if (prevStats[i].user != currStats[i].user ||
                prevStats[i].nice != currStats[i].nice ||
                prevStats[i].system != currStats[i].system ||
                prevStats[i].idle != currStats[i].idle ||
                prevStats[i].iowait != currStats[i].iowait ||
                prevStats[i].irq != currStats[i].irq ||
                prevStats[i].softirq != currStats[i].softirq ||
                prevStats[i].steal != currStats[i].steal ||
                prevStats[i].guest != currStats[i].guest ||
                prevStats[i].guest_nice != currStats[i].guest_nice) {
                threadsRunning = true;
                break;
            }
        }

        if (!threadsRunning) {
            isRunning = false;
            printf("Error: Threads have stopped running. Application terminated.\n");
        }
    }
    pthread_exit(NULL);
}

static void *loggerThread(void *arg) {
    LoggerThreadData *loggerData = (LoggerThreadData *) arg;
    FILE *logFile = loggerData->logFile;

    while (isRunning) {
        ThreadData *data = loggerData->data;
        pthread_mutex_lock(&(data->mutex));
        double *usage = data->usage;
        long numOfCores = data->numOfCores;
        pthread_mutex_unlock(&(data->mutex));

        fprintf(logFile, "Overall CPU Usage: %.2f%%\n", usage[0]);
        for (short i = 0; i < numOfCores; i++) {
            fprintf(logFile, "Core %d Usage: %.2f%%\n", i, usage[i + 1]);
        }
        sleep(1);
        fflush(logFile);
    }
    pthread_exit(NULL);
}

static void sigHandler(int signum) {
    printf("\nCaught signal %d\n", signum);
    isRunning = false;
}

int main(void) {
    long numOfCores = sysconf(_SC_NPROCESSORS_ONLN);

    ThreadData data;
    data.numOfCores = numOfCores;
    data.prevStats = malloc(sizeof(CpuStats) * (unsigned long) numOfCores);
    data.currStats = malloc(sizeof(CpuStats) * (unsigned long) numOfCores);
    data.usage = malloc(sizeof(double) * (unsigned long) (numOfCores + 1));

    pthread_mutex_init(&(data.mutex), NULL);

    pthread_t readerThreadId, analyzerThreadId, printerThreadId, watchdogThreadId, loggerThreadId;
    pthread_create(&readerThreadId, NULL, readerThread, &data);
    pthread_create(&analyzerThreadId, NULL, analyzerThread, &data);
    pthread_create(&printerThreadId, NULL, printerThread, &data);
    pthread_create(&watchdogThreadId, NULL, watchdogThread, &data);

    FILE *logFile = fopen("log.txt", "w");
    if (logFile == NULL) {
        perror("Failed to open log file");
        exit(1);
    }

    LoggerThreadData loggerData;
    loggerData.data = &data;
    loggerData.logFile = logFile;
    pthread_create(&loggerThreadId, NULL, loggerThread, &loggerData);

    signal(SIGTERM, sigHandler);
    signal(SIGINT, sigHandler);

    pthread_join(readerThreadId, NULL);
    pthread_join(analyzerThreadId, NULL);
    pthread_join(printerThreadId, NULL);
    pthread_join(watchdogThreadId, NULL);
    pthread_join(loggerThreadId, NULL);

    pthread_mutex_destroy(&(data.mutex));
    free(data.prevStats);
    free(data.currStats);
    free(data.usage);
    fclose(logFile);

    return 0;
}
