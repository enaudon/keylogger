//#include <linux/kernel.h>
//#include <linux/mm.h>
//#include <linux/slab.h>
//#include <asm/uaccess.h>
//#include <linux/smp_lock.h>
//#include <asm/semaphore.h>
#include <linux/init.h>
#include <linux/mman.h>
#include <linux/module.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
MODULE_LICENSE("DUAL BSD/GPL");

#define JMP_BYTES 7

//sys_getpid address (system-specific)
long (*getpid)(void) = (void *)0xC1067F20;

//locks
spinlock_t gp_spin = SPIN_LOCK_UNLOCKED;

//original and replacement instructions at sys_getpid
static char orig_instr[JMP_BYTES];
static char repl_instr[JMP_BYTES] = "\xb8\x00\x00\x00\x00"  //mov eax, 0
                                    "\xff\xe0";             //jmp eax

/*Copies bytes from the specified source to the specified destination.
 */
void *_memcpy(void *dest, const void *src, int bytes) {
  const char *s = src;
  char *d = dest;

  //swap bytes
  register int i;
  for (i = 0; i < bytes; ++i) *d++ = *s++;

  return dest;
}

/*New sys_getpid function.
 *This is a proof of concept, so there's no need to be mean here--just print an
 *alert.
 */
asmlinkage long hacked_getpid(void) {
  long pid;  //sys_getpid return value

  printk(KERN_ALERT "All your base are belong to us.\n");

  /* Critical Section */
  spin_lock(&gp_spin);

  //remove write protection
  write_cr0 (read_cr0 () & (~0x10000));

  //restore getpid, call it and overwrite it again
  _memcpy(getpid, orig_instr, JMP_BYTES);  //restore
  pid = getpid();                          //call
  _memcpy(getpid, repl_instr, JMP_BYTES);  //overwrite

  //reenable write protection
  write_cr0 (read_cr0 () | 0x10000);

  spin_unlock(&gp_spin);
  /* End Critical Section */

  return pid;
}

/*Module initialization function.
 */
static int hello_init(void) {

  printk(KERN_ALERT "Hello, world!\n");

  //get addr of hacked_getpid into asm framework
  *(long *)&repl_instr[1] = (unsigned)hacked_getpid;

  //save the bytes at getpid
  _memcpy(orig_instr, getpid, JMP_BYTES);


  //remove write protection
  write_cr0 (read_cr0 () & (~0x10000));

  //THIS LINE CAUSES THE CRASH (i think...)
  _memcpy(getpid, repl_instr, JMP_BYTES);

  //reenable write protection
  write_cr0 (read_cr0 () | 0x10000);

  return 0;
}

/*Module clean-up function.
 */
static void hello_exit(void) {

  //remove write protection
  write_cr0 (read_cr0 () & (~0x10000));

  //replace original getpid bytes
  _memcpy(getpid, orig_instr, JMP_BYTES);

  //reenable write protection
  write_cr0 (read_cr0 () | 0x10000);

  printk(KERN_ALERT "Goodbye, world!\n");
}

module_init(hello_init);
module_exit(hello_exit);
