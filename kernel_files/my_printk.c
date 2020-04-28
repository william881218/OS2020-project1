#include <linux/linkage.h>
#include <linux/kernel.h>

asmlinkage void sys_my_printk(char *message){
    printk("%s\n", message);
}
