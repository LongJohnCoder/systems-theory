#include <linux/module.h>  /* Needed by all modules */
#include <linux/kernel.h>  /* Needed for KERN_ALERT */
#include <linux/timer.h>   /* Needed for timer */

MODULE_AUTHOR ("Eike Ritter <E.Ritter@cs.bham.ac.uk>");
MODULE_DESCRIPTION ("Simple timer usage") ;
MODULE_LICENSE("GPL");

spinlock_t my_lock = SPIN_LOCK_UNLOCKED; /* the lock for the counter */

/* setup timer */
struct timer_list myTimer;
int i;

void timerFun (unsigned long arg) {
    int tmp;
    /* In this simple example, locking is an overkill - this operation should finish within a second  - used here to demonstrate how it works */
    spin_lock (&my_lock);
    i++;
    tmp = i;
    spin_unlock (&my_lock);
    printk (KERN_INFO "Called timer %d times\n", tmp); 
    myTimer.expires = jiffies + HZ;
    add_timer (&myTimer); /* setup the timer again */
}

int init_module(void)
{

    /* pre-defined kernel variable jiffies gives current value of ticks */
    unsigned long currentTime = jiffies; 
    unsigned long expiryTime = currentTime + HZ; /* HZ gives number of ticks per second */
    init_timer (&myTimer);
    myTimer.function = timerFun;
    myTimer.expires = expiryTime;
    myTimer.data = 0;

    add_timer (&myTimer);
    printk (KERN_INFO "timer added \n");
    return 0;
}

void cleanup_module (void) {
    if (!del_timer (&myTimer)) {
	printk (KERN_INFO "Couldn't remove timer!!\n");
    }
    else {
	printk (KERN_INFO "timer removed \n");
    }
}



    
    

    
    
