#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include "main.h"

static void testCpuStats(void) {
    CpuStats stats;
    readCpuStats(&stats, 0);
    assert(stats.user >= 0);
    assert(stats.nice >= 0);
    assert(stats.system >= 0);
    assert(stats.idle >= 0);
    assert(stats.iowait >= 0);
    assert(stats.irq >= 0);
    assert(stats.softirq >= 0);
    assert(stats.steal >= 0);
    assert(stats.guest >= 0);
    assert(stats.guest_nice >= 0);
}

int main(void) {
    testCpuStats();
    printf("Unit tests passed\n");
    sleep(1);
}
