#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/timer.h>

asmlinkage void my_get_time(unsigned long *time_s, unsigned long *time_ns) {
    struct timespec t;
    getnstimeofday(&t);
    *time_s = t.tv_sec
    *time_ns = t.tv_nsec;
    return;
}
