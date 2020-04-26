#include <linux/linkage.h>
#include <linux/kernel.h>

asmlinkage void my_printk(char *message) {
    printk("%s\n", message);
}
