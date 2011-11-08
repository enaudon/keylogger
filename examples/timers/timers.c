#include <linux/init.h>
#include <linux/module.h>

#include <linux/timer.h>

typedef struct timer_list timer;
 
//our timer struct
static timer t;

/*Called when the timer fires.
 *
 *@param data timer.data (not 100% sure why this is useful)
 */
void timer_listener(unsigned long data) {
  printk(KERN_ALERT "HAI!\n");

  //again, again!
  mod_timer(&t, jiffies + msecs_to_jiffies(5000));
}

static int hello_init(void) {
  printk(KERN_ALERT "Hello, world!\n");

  //initialize and prime the timer
  setup_timer(&t, timer_listener, 1234);
  mod_timer(&t, jiffies + msecs_to_jiffies(5000));

  return 0;
}

static void hello_exit(void) {

  //remove our timer
  del_timer(&t);

  printk(KERN_ALERT "Goobye, world!\n");
}

MODULE_LICENSE("GPL");
module_init(hello_init);
module_exit(hello_exit);
