/*  Example for transferring data from kernel into user space
 */

#include <linux/module.h>  /* Needed by all modules */
#include <linux/kernel.h>  /* Needed for KERN_ALERT */
#include <linux/proc_fs.h> 
#include <asm/uaccess.h>
#include <linux/list.h>
#include <linux/slab.h>
#include "entry.h"

#define BUFFERSIZE 10

#define PROC_ENTRY_FILENAME "kernelRead"


MODULE_AUTHOR ("Eike Ritter <E.Ritter@cs.bham.ac.uk>");
MODULE_DESCRIPTION ("Reading data from kernel") ;
MODULE_LICENSE("GPL");

int counter = 0; /* number of entries to be transferred in total */

static struct proc_dir_entry *Our_Proc_File;


/* the function called to write data into the proc-buffer */
ssize_t kernelRead (struct file *fp,
		 char __user *buffer,  /* the destination buffer */
		 size_t buffer_size,  /* size of buffer */
		 loff_t *offset  /* offset in destination buffer */
	        ) {
  char *pos;    /* the current position in the buffer */
  struct entry_t *entry;
  static int finished = 0;
  int retval = 0;  /* number of bytes read; return value for function */
  int i = 0;
  printk (KERN_INFO "procfile_read called with offset of %lld and buffer size %ld\n",  *offset, buffer_size);



  if (finished) {
      printk (KERN_INFO "procfs_read: END\n");
      finished = 0;
      return 0;
  }
  
  entry = kmalloc (sizeof (struct entry_t), GFP_KERNEL);
  if (!entry) {
      return -EFAULT;
  }

  printk (KERN_INFO "The (kernel space address of entry is %lx\n", (unsigned long) entry);
  printk (KERN_INFO "The user space address of the buffer is %lx\n", (unsigned long) buffer);

  pos = buffer;
  while (pos + sizeof (struct entry_t) <= buffer + buffer_size ) {
    entry->field1 = i;  /* create some data */
    entry->field2 = -i; /* create some data */
    /* copy it into user buffer */
    if (copy_to_user (pos, entry, sizeof (struct entry_t))) {
	kfree (entry);
	return -EFAULT;
    }
	    
    pos += sizeof (struct entry_t); /* increase the counters */
    counter++;
    i ++;
    retval = retval + sizeof (struct entry_t);
  }
  if (counter == BUFFERSIZE) {
      finished = 1;
      counter = 0;
  }
  printk (KERN_INFO "procfile read returned %d byte\n", retval);
  kfree (entry);
  return retval;
}


/* 
 * The file is opened - we don't really care about
 * that, but it does mean we need to increment the
 * module's reference count. 
 */
int procfs_open(struct inode *inode, struct file *file)
{
    printk (KERN_INFO "kernelRead opened\n");
	try_module_get(THIS_MODULE);
	return 0;
}

/* 
 * The file is closed - again, interesting only because
 * of the reference count. 
 */
int procfs_close(struct inode *inode, struct file *file)
{
    printk (KERN_INFO "kernelRead closed\n");
	module_put(THIS_MODULE);
	return 0;		/* success */
}

const struct file_operations File_Ops_4_Our_Proc_File = {
    .owner = THIS_MODULE,
    .read 	 = kernelRead,
    .open 	 = procfs_open,
    .release = procfs_close,
};


int init_module (void) {

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

void cleanup_module (void) {
  remove_proc_entry (PROC_ENTRY_FILENAME, NULL);  /* now, no further module calls possible */
  printk (KERN_INFO "kernel Read  removed \n");

}
