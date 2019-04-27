/*  Example for transferring data from user space into the kernel 
 */

#include <linux/module.h>  /* Needed by all modules */
#include <linux/kernel.h>  /* Needed for KERN_ALERT */
#include <linux/proc_fs.h> 
#include <asm/uaccess.h>
#include <linux/list.h>

MODULE_AUTHOR ("Eike Ritter <E.Ritter@cs.bham.ac.uk>");
MODULE_DESCRIPTION ("Reading data") ;
MODULE_LICENSE("GPL");

#define BUFFERLENGTH 256
#define ADD_ENTRY 'A'
#define SHOW_TABLE 'S'

DECLARE_RWSEM(list_sem); /* semaphore to protect list access */

struct proc_dir_entry *procKernelRead; /* the handler for the proc-file */

struct lineList {
  char *line;
  struct lineList *next;
}; /* the list-structure for keeping the data in the kernel */

struct lineList *kernelList = NULL; /* the global list of words */

/* adds line from user space to the list kept in kernel space */
struct lineList *add_entry (struct lineList *lineList, char *line) {

  struct lineList *newEntry;
  /* allocate memory for new list element */
  newEntry = kmalloc (sizeof (struct lineList), GFP_KERNEL);
  if (!newEntry) {
    return NULL;
  }

  newEntry->line = line;

  /* protect list access via semaphore */
  down_write (&list_sem);
  newEntry->next = lineList;
  lineList = newEntry;
  up_write (&list_sem);

  /* return new list */
  return lineList;

}

/* displays the kernel table - for simplicity via printk */
void show_table (struct lineList *lineList) {

  struct lineList *tmp;
  down_read (&list_sem); /* lock for reading */
  tmp = lineList;
  while (tmp) {
    printk (KERN_INFO "kernelRead:The next entry is %s\n", tmp->line);
    tmp = tmp->next;
  }
  up_read (&list_sem); /* unlock reading */

}

/* This function reads in data from the user into the kernel */
int kernelRead (struct file *file, const char *buffer, unsigned long count, void *data) { 


  char *kernelBuffer; /* the kernel buffer */
 
  
  struct lineList *tmp;

  kernelBuffer = kmalloc (BUFFERLENGTH, GFP_KERNEL); /* allocate memory */
   
  if (!kernelBuffer) {
    return -ENOMEM;
  }


  if (count > BUFFERLENGTH) { /* make sure we don't get buffer overflow */
    kfree (kernelBuffer);
    return -EFAULT;
  }

  
  /* copy data from user space */
  if (copy_from_user (kernelBuffer, buffer, count)) { 
    kfree (kernelBuffer);
    return -EFAULT;
  }
      
  kernelBuffer[BUFFERLENGTH -1]  ='\0'; /* safety measure: ensure string termination */
  printk (KERN_INFO "kernelRead: Having read %s\n", kernelBuffer);

  switch (kernelBuffer[0]) {
    case ADD_ENTRY:
      tmp = add_entry (kernelList, &(kernelBuffer[1]));
      if (!tmp) {
	kfree (kernelBuffer);
	return -EFAULT;
      }
      else {
	kernelList = tmp;
      }
      break;
    case SHOW_TABLE:
      show_table (kernelList);
      break;
    default: 
      printk (KERN_INFO "kernelRead: Illegal command \n");
  }
  return count;
}
  


int init_module(void)
{
   printk(KERN_INFO "kernelRead:Proc module loaded.\n");
	

   /* create the proc-file */
   procKernelRead = create_proc_entry ("kernelRead", S_IWUSR | S_IRUGO, NULL);

   if (!procKernelRead) {
     return -ENOMEM;
   }


   /* specify the procedure which reads data */
   procKernelRead->write_proc = kernelRead;


   return 0;
}
  
void cleanup_module(void)
{

  struct lineList *tmp;

  remove_proc_entry ("kernelRead", NULL);  /* now, no further module calls possible */
  
  /* free the list */
  while (kernelList) {
    tmp = kernelList->next;
    kfree (kernelList->line);
    kfree (kernelList);
    kernelList = tmp;
  }
  
  printk(KERN_INFO "kernelRead:Proc module unloaded.\n");
  
}  
