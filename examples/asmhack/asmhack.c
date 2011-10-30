#include <linux/init.h>
#include <linux/module.h>
MODULE_LICENSE("DUAL BSD/GPL");

#define JMP_BYTES 7

//sys_getpid address (system-specific)
long (*getpid)(void) = (void *)0xc1067f20;

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
  printk(KERN_ALERT "All your base are belong to... me?\n");
  return getpid();
}

/*Module initialization function.
 */
static int hello_init(void) {

  //get addr of hacked_getpid into asm framework
  printk(KERN_ALERT "%X\n", repl_instr[1]);
  *(long *)&repl_instr[1] = (long)hacked_getpid;
  printk(KERN_ALERT "%X\n", (long)hacked_getpid);
  printk(KERN_ALERT "%X\n", repl_instr[1]);

  return 0;
}

/*Module clean-up function.
 */
static void hello_exit(void) {
  printk(KERN_ALERT "Goobye, world!\n");
}

module_init(hello_init);
module_exit(hello_exit);
