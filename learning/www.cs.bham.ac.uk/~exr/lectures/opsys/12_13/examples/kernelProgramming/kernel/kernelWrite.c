/*  Example for transferring data from kernel into user space
 */

#include <linux/module.h>  /* Needed by all modules */
#include <linux/kernel.h>  /* Needed for KERN_ALERT */
#include <linux/proc_fs.h> 
#include <asm/uaccess.h>
#include <linux/list.h>
#include "entry.h"

#define BUFFERSIZE 10

MODULE_AUTHOR ("Eike Ritter <E.Ritter@cs.bham.ac.uk>");
MODULE_DESCRIPTION ("Writing data") ;
MODULE_LICENSE("GPL");

struct proc_dir_entry *procKernelWrite;
int counter = 0; /* number of entries to be transferred in total */

/* the function called to write data into the proc-buffer */
int kernelWrite (
		 char *buffer,  /* the destination buffer (in kernel space */
		 char **start,  /* not used in the kernel */
		 off_t offset,  /* offset in destination buffer */
		 int buffer_size,  /* size of buffer */
		 int *eof,      /* write 1 to indicate EOF */
		 void *data     /* additional argument to be used at discretion of function */
	        ) {
  char *pos;    /* the current position in the buffer */
  struct entry_t entry;
  int retval = 0;  /* number of bytes read; return value for function */
  int i = 0;
  printk (KERN_INFO "procfile_read called with offset of %d and buffer size %d\n", (int) offset, buffer_size);

  pos = buffer;
  while (pos + sizeof (struct entry_t) <= buffer + buffer_size ) {
    entry.field1 = i;  /* create some data */
    entry.field2 = -i; /* create some data */
    memcpy (pos, &entry, sizeof (struct entry_t)); /* copy it into buffer */
    pos += sizeof (struct entry_t); /* increase the counters */
    counter++;
    i ++;
    retval = retval + sizeof (struct entry_t);
  }
  if (counter == BUFFERSIZE) {
    *eof = 1;
    counter = 0;
  }
  printk (KERN_INFO "procfile read returned %d byte\n", retval);
  return retval;
}

int init_module (void) {

   procKernelWrite = create_proc_entry ("kernelWrite", S_IWUSR | S_IRUGO, NULL);

   if (!procKernelWrite) {
     return -ENOMEM;
   }


   /* specify the procedure which reads data */
   procKernelWrite->read_proc = kernelWrite;
   printk (KERN_INFO "kernel Write added \n");
   return 0;

}

void cleanup_module (void) {
  remove_proc_entry ("kernelWrite", NULL);  /* now, no further module calls possible */
   printk (KERN_INFO "kernel Write removed \n");

}

  
