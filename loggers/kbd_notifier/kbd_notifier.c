#include <linux/init.h>
#include <linux/module.h>

#include <linux/keyboard.h>

int kbd_listener(struct notifier_block *, unsigned long, void *);

//notifier block for kbd_listener
struct notifier_block nb = {
  .notifier_call = kbd_listener
};

/*Called when a key is pressed.
 *No, I don't know what the parameters are/do.
 */
int kbd_listener(struct notifier_block *nb, unsigned long num, void *ptr) {
  //printk(KERN_ALERT "Notified!\n");
  struct keyboard_notifier_param *keystroke_data = ptr;
  unsigned int val = keystroke_data->value;

  switch (num) {
    case KBD_KEYCODE :
      printk(KERN_ALERT "Keystroke!\n"); break;
    case KBD_UNBOUND_KEYCODE:
      printk(KERN_ALERT "Unbound\n"); break;
    case KBD_UNICODE :
      printk(KERN_ALERT "Unicode?\n"); break;
    case KBD_KEYSYM :
      printk(KERN_ALERT "Keysym  : %x\n", keystroke_data->value);
      printk(KERN_ALERT "          %x,%x\n", KTYP(val), KVAL(val)); break;
    case KBD_POST_KEYSYM :
      printk(KERN_ALERT "Post KS : %x\n", keystroke_data->value);
      printk(KERN_ALERT "          %x,%x\n", KTYP(val), KVAL(val)); break;
    default :
      printk(KERN_ALERT "Really not sure what happend\n"); break;
  }

  return 0;
}

static int hello_init(void) {
  printk(KERN_ALERT "Hello, world!\n");

  //notice when a key is pressed?  yes, please!
  register_keyboard_notifier(&nb);

  return 0;
}

static void hello_exit(void) {

  //unregister kbd_listener
  unregister_keyboard_notifier(&nb);

  printk(KERN_ALERT "Goobye, world!\n");
}

MODULE_LICENSE("GPL");
module_init(hello_init);
module_exit(hello_exit);
