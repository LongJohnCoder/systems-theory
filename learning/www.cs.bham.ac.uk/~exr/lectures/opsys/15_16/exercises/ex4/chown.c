/* TODO: Implement three functions, namely minix_chown, process_inodes and proc_chown */


/* minix_chown: 

sets uid-field for given inode to specified value


*/

/* process_inodes: 
   
  goes through the map of valid inodes and for each valid inode calls minix_chown

 */

/* proc_chown 

   handles writing to the proc-file. It processes the string passed to it, and calls minix_chown or process_inodes as appropriate.

 */
