#include <linux/kernel.h>

asmlinkage long sys_hello (void) {
    printk ("Hello World\n");
    return 0;
}

