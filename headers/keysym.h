/* =============================
 * Key Symbol Translation Header
 * =============================
 *
 * Description:
 * -----------
 *      This header files contains a variety of translation functions (one for
 * each used key symbol type), along with a wrapper function (xlate_ksym) that
 * can translate any key symbol.  The type-specific translation functions
 * should not be directly invoked.  The wrapper function will select the
 * appropriate function to translate a given keystroke.  The wrapper takes two
 * parameters, a pointer to a keyboard_notifier_param and a pointer to an
 * character buffer where the keystroke's string representation should be
 * stored:
 *    void xlate_keysym(keyboard_notifier_param *, char *)
 *
 * Also, note that this file contains a typedef of the keyboard_notifier_param
 * struct to make it easier to deal with; you can call it keystroke_data
 * instead if you like.
 *
 * The keyboard_notifier_param Structure:
 * -------------------------------------
 *      They keyboard_notifier_param structure and some of its fields are a
 * little confusing and deserve some mention.  According to LXR, the
 * keyboard_notifier_param struct is defined as follows in linux v2.6.38-8:
 *
 *    struct keyboard_notifier_param {
 *      struct vc_data *vc;     // VC on which the keyboard press was done
 *      int down;               // Pressure of the key?
 *      int shift;              // Current shift mask
 *      int ledstate;           // Current led state
 *      unsigned int value;     // keycode, unicode value or keysym
 *    };
 *
 * The important fields for our purposes are...
 *    1) down: flag that indicates the pressure of the keyboard event
 *        - zero     = key-press
 *        - non-zero = key-release
 *    2) ledstate: set of binary flags indicating the state of scoll and number
 *                 lock 
 *        - set     = scroll/num lock enabled
 *        - cleared = scroll/num lock disabled
 *        - bit 0: scroll lock
 *        - bit 1: num lock
 *    3) value: (arbitrary) short used to identify each key
 *        - interpretation depends on keyboard mode (we assume non-unicode)
 *        - high-order byte is a type; low-order byte is a value
 *        - type used to select the appropriate translation function
 *        - value used to perform the actual translation
 */

#include <linux/keyboard.h>

//ledstate bitmasks
#define SLOCK_MASK 0x0000001  //bit 0 = scroll lock
#define NLOCK_MASK 0x0000002  //bit 1 = num lock

//string representations for common keys
#define UNKNOWN "<unkn>"
#define NO_EFCT "<null>"
#define PG_UP   "<pg up>"
#define PG_DN   "<pg dn>"
#define HOME    "<home>"
#define END     "<end>"
#define ARW_UP  "<u arw>"
#define ARW_DN  "<d arw>"
#define ARW_LT  "<l arw>"
#define ARW_RT  "<r arw>"

typedef struct keyboard_notifier_param keystroke_data;

/*Type-specific translation function prototypes.
 *
 *Parameters:
 *  ks   - keystroke_data (keyboard_notifier_param)
 *  buf  - string representation output buffer
 *Returns:
 *  none - result in output buffer
 */
static void ksym_num(keystroke_data *ks, char *buf);
static void ksym_mod(keystroke_data *ks, char *buf);
static void ksym_std(keystroke_data *ks, char *buf);

//key symbol to string tables
char *locked_npad[17] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
                         "+", "-", "*", "/", "<enter>", UNKNOWN, "."};
char *unlocked_npad[17] = {"<ins>", END, ARW_DN, PG_DN, ARW_LT, NO_EFCT, 
                           ARW_RT, HOME, ARW_UP, PG_UP, "+", "-", "*",
                           "/", "<enter>", UNKNOWN, "."};
char *mods[4] = {"<shift _>", UNKNOWN, "<ctrl _>", "<alt _>"};

/*Wrapper for key symbol translation fucntions.
 *Parses type and selects the appropriate translation function.
 *
 *Parameters:
 *  ks_param - keystroke data
 *  buf      - output buffer
 *Returns:
 *  ni nada
 */
void xlate_keysym(keystroke_data *ks_param, char *buf) {
  unsigned char type = ks_param->value >> 8;

  //high nybble doesn't seem to be useful
  type &= 0x0f;

  //select appropriate translation function
  switch (type) {
    case 0x0 : 
    case 0x1 :
    case 0x2 :
    case 0x3 : ksym_num(ks_param, buf);  break;
    case 0x4 :
    case 0x5 :
    case 0x6 :
    case 0x7 : ksym_mod(ks_param, buf);  break;
    case 0x8 :
    case 0x9 :
    case 0xa :
    case 0xb : ksym_std(ks_param, buf);  break;
    case 0xc :
    case 0xd :
    case 0xe :
    case 0xf : break;
  }
}

/*Translate normal keys.
 *Return the value on key-press; ignore key-release.
 */
static void ksym_std(keystroke_data *ks, char *buf) {
  if (ks->down) sprintf(buf, "%c", ks->value);
  else          buf[0] = 0x00;
}

/*Translate normal modifier keys.
 *Return the appropriate string on both key-press and key-release events.  An
 *event-specific character is added to the strings (p for press, r for
 *release events).
 */
static void ksym_mod(keystroke_data *ks, char *buf) {
  int len;
  unsigned char val  = ks->value & 0x00ff;

  //just in case
  if (val > 3) return;

  //translate mod key to string
  len = sprintf(buf, "%s", mods[val]);

  //add pressure indicator
  buf[len - 2] = ks->down ? 'p' : 'r';
}

/*Translate number pad keys.
 *The ledstate field of the keystroke param must be parsed to determine
 *whether numlock is enabled or not.  See defines for more info on how
 *ledstate is handled.
 */
static void ksym_num(keystroke_data *ks, char *buf) {
  unsigned char val  = ks->value & 0x00ff;

  //just in case
  if (val > 16) return;

  //ignore key-release events
  if (!ks->down) {
    buf[0] = 0x00;
    return;
  }

  //values depend on the state of numlock
  if (ks->ledstate & NLOCK_MASK)
    sprintf(buf, "%s", locked_npad[val]);
  else if (!(ks->ledstate & NLOCK_MASK))
    sprintf(buf, "%s", unlocked_npad[val]);
}

int disp(unsigned char value) {
  return (value > 0x19 && value < 0x7F);
}
