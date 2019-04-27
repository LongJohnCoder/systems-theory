/*  hello.c - The simplest kernel module.
 */

#include <linux/module.h>  /* Needed by all modules */
#include <linux/kernel.h>  /* Needed for KERN_ALERT */
#include <linux/namei.h>
#include <linux/sched.h>


#define BUFFERSIZE 80

MODULE_AUTHOR ("Eike Ritter <E.Ritter@cs.bham.ac.uk>");
MODULE_DESCRIPTION ("Finding executable") ;
MODULE_LICENSE("GPL");



int init_module(void)
{
    struct path path;
    pid_t mod_pid;
    struct dentry *procDentry;
    struct dentry *parent;

    char cmdlineFile[BUFFERSIZE];
    int res;
    
    printk (KERN_INFO "findExecutable module loading\n");
    /* current is pre-defined pointer to task structure of currently running task */
    mod_pid = current->pid;
    snprintf (cmdlineFile, BUFFERSIZE, "/proc/%d/exe", mod_pid); 
    res = kern_path (cmdlineFile, LOOKUP_FOLLOW, &path);
    if (res) {
	printk (KERN_INFO "Could not get dentry for %s!\n", cmdlineFile);
	return -EFAULT;
    }
    
    procDentry = path.dentry;
    printk (KERN_INFO "The name is %s\n", procDentry->d_name.name);
    parent = procDentry->d_parent;
    printk (KERN_INFO "The name of the parent is %s\n", parent->d_name.name);
    return 0;
    
}


void cleanup_module(void)
{
    printk(KERN_INFO "findExecutable module unloading \n");
}  
