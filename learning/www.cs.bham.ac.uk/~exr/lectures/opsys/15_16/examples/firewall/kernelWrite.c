/*  Example for transferring data from user space into the kernel 
 */

#include <linux/module.h>  /* Needed by all modules */
#include <linux/kernel.h>  /* Needed for KERN_ALERT */
#include <linux/proc_fs.h> 
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/list.h>

MODULE_AUTHOR ("Eike Ritter <E.Ritter@cs.bham.ac.uk>");
MODULE_DESCRIPTION ("Writing data to kernel") ;
MODULE_LICENSE("GPL");

#define BUFFERLENGTH 256
#define INCREASE_COUNTER 'I'
#define SHOW_COUNTER 'S'

#define PROC_ENTRY_FILENAME "kernelWrite"



DECLARE_RWSEM(counter_sem); /* semaphore to protect counter access */

static struct proc_dir_entry *Our_Proc_File;

int counter1 = 0;
int counter2 = 0;

/* displays the kernel table - for simplicity via printk */
void show_table (void) {

    int tmp1;
    int tmp2;

    down_read (&counter_sem); /* lock for reading */
    tmp1 = counter1;
    tmp2 = counter2;
    up_read (&counter_sem); /* unlock reading */
    printk (KERN_INFO "kernelWrite:The counters are %d, %d\n", tmp1, tmp2);

}

void increase_counter (void) {
    
    down_write (&counter_sem); /* lock for writing */
    counter1++;
    counter2++;
    up_write (&counter_sem);
    
}

/* This function reads in data from the user into the kernel */
ssize_t kernelWrite (struct file *file, const char __user *buffer, size_t count, loff_t *offset) {


    char command;

    printk (KERN_INFO "kernelWrite entered\n");


    if (get_user (command, buffer)) {
	return -EFAULT;
    }
  
  switch (command) {
    case INCREASE_COUNTER:
	increase_counter ();
	break;
    case SHOW_COUNTER:
	show_table ();
      break;
    default: 
      printk (KERN_INFO "kernelWrite: Illegal command \n");
  }
  return count;
}
  


/* 
 * The file is opened - we don't really care about
 * that, but it does mean we need to increment the
 * module's reference count. 
 */
int procfs_open(struct inode *inode, struct file *file)
{
    printk (KERN_INFO "kernelWrite opened\n");
	try_module_get(THIS_MODULE);
	return 0;
}

/* 
 * The file is closed - again, interesting only because
 * of the reference count. 
 */
int procfs_close(struct inode *inode, struct file *file)
{
    printk (KERN_INFO "kernelWrite closed\n");
    module_put(THIS_MODULE);
    return 0;		/* success */
}

const struct file_operations File_Ops_4_Our_Proc_File = {
    .owner = THIS_MODULE,
    .write 	 = kernelWrite,
    .open 	 = procfs_open,
    .release = procfs_close,
};


int init_module(void)
{
	

    
    /* create the /proc file */
    Our_Proc_File = proc_create_data (PROC_ENTRY_FILENAME, 0644, NULL, &File_Ops_4_Our_Proc_File, NULL);
    
    /* check if the /proc file was created successfuly */
    if (Our_Proc_File == NULL){
	printk(KERN_ALERT "Error: Could not initialize /proc/%s\n",
	       PROC_ENTRY_FILENAME);
	return -ENOMEM;
    }
    
    printk(KERN_INFO "/proc/%s created\n", PROC_ENTRY_FILENAME);
    
    return 0;	/* success */

}
  
void cleanup_module(void)
{

  remove_proc_entry(PROC_ENTRY_FILENAME, NULL);
  printk(KERN_INFO "/proc/%s removed\n", PROC_ENTRY_FILENAME);  

  printk(KERN_INFO "kernelWrite:Proc module unloaded.\n");
  
}  
