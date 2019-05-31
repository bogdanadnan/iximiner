//
// Created by Haifa Bogdan Adnan on 31/05/2019.
//

#ifndef IXIMINER_CPU_AFFINITY_H
#define IXIMINER_CPU_AFFINITY_H

#define SYSCTL_CORE_COUNT   "machdep.cpu.core_count"

typedef struct cpu_set {
    uint32_t    count;
} cpu_set_t;

static inline void
CPU_ZERO(cpu_set_t *cs) { cs->count = 0; }

static inline void
CPU_SET(int num, cpu_set_t *cs) { cs->count |= (1 << num); }

static inline int
CPU_ISSET(int num, cpu_set_t *cs) { return (cs->count & (1 << num)); }

DLLEXPORT int sched_getaffinity(pid_t pid, size_t cpu_size, cpu_set_t *cpu_set);
DLLEXPORT int pthread_setaffinity_np(pthread_t thread, size_t cpu_size, cpu_set_t *cpu_set);

#endif //IXIMINER_CPU_AFFINITY_H
