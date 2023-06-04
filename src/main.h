#ifndef CPU_USAGE_TRACKER_MAIN_H
#define CPU_USAGE_TRACKER_MAIN_H

#define MAX_LINE_LENGTH 256
#define MAX_LABEL_LENGTH 8

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

void readCpuStats(CpuStats *stats, int core);

#endif
