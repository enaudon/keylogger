#include <linux/init.h>
#include <linux/module.h>
MODULE_LICENSE("DUAL BSD/GPL");

static int hello_init(void) {
  printk(KERN_ALERT "Hello, world!\n");
  return 0;
}

static void hello_exit(void) {
  printk(KERN_ALERT "Goobye, world!\n");
}

module_init(hello_init);
module_exit(hello_exit);
