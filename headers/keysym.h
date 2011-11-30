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

//maximum output buffer size
#define BUFLEN 65535

//ledstate bitmasks
#define SLOCK_MASK 0x0000001  //bit 0 = scroll lock
#define NLOCK_MASK 0x0000002  //bit 1 = num lock

//string representations for common keys
#define UNKNOWN "<unkn>"
#define NO_EFCT "<null>"
#define ENTER   "<ent>"
#define INSERT  "<ins>"
#define DELETE  "<del>"
#define PAGE_UP "<pg up>"
#define PAGE_DN "<pg dn>"
#define HOME    "<home>"
#define END     "<end>"
#define PAU_BRK "<pb>"
#define ARW_UP  "<u arw>"
#define ARW_DN  "<d arw>"
#define ARW_LT  "<l arw>"
#define ARW_RT  "<r arw>"
#define F_KEYS  "<f"  //no, i didn't forget the end

typedef struct keyboard_notifier_param keystroke_data;

/*Type-specific translation function prototypes.
 *
 *Parameters:
 *  ks   - keystroke_data (keyboard_notifier_param)
 *  buf  - string representation output buffer
 *
 *Returns:
 *  none - result in output buffer
 */
static void ksym_std(keystroke_data *ks, char *buf);
static void ksym_fnc(keystroke_data *ks, char *buf);
static void ksym_num(keystroke_data *ks, char *buf);
static void ksym_mod(keystroke_data *ks, char *buf);

//key symbol to string tables
//the ascii table should be enlarged to support an extended character set
//(perhaps a common one like latin-1?)
char *ascii[128] = {
  NO_EFCT, "<SOH>", "<STX>", "<ETX>", "<EOT>", "<ENQ>", "<ACK>", "<BEL>",
  "<BS>",  "<TAB>", "<LF>",  "<VT>",  "<FF>",  "<CR>",  "<SO>",  "<SI>",
  "<DLE>", "<DC1>", "<DC2>", "<DC3>", "<DC4>", "<NAK>", "<SYN>", "<ETB>",
  "<CAN>", "<EM>",  "<SUB>", "<ESC>", "<FS>",  "<GS>",  "<RS>",  "<US>",
  " ","!","\"","#","$","%","&","'","(",")","*","+",",", "-",".","/",
  "0","1","2", "3","4","5","6","7","8","9",":",";","<", "=",">","?",
  "@","A","B", "C","D","E","F","G","H","I","J","K","L", "M","N","O",
  "P","Q","R", "S","T","U","V","W","X","Y","Z","[","\\","]","^","_",
  "`","a","b", "c","d","e","f","g","h","i","j","k","l", "m","n","o",
  "p","q","r", "s","t","u","v","w","x","y","z","{","|", "}","~","<DEL>"};
char *fncs[16] = {UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
                  HOME, INSERT, DELETE, END,
                  PAGE_UP, PAGE_DN, UNKNOWN, UNKNOWN,
                  UNKNOWN, PAU_BRK, UNKNOWN, UNKNOWN};
char *locked_npad[17] = {"0", "1", "2", "3",
                         "4", "5", "6", "7",
                         "8", "9", "+", "-",
                         "*", "/", ENTER, UNKNOWN,
                         "."};
char *unlocked_npad[17] = {INSERT, END, ARW_DN, PAGE_DN,
                           ARW_LT, NO_EFCT, ARW_RT, HOME,
                           ARW_UP, PAGE_UP, "+", "-",
                           "*", "/", ENTER, UNKNOWN,
                           DELETE};
char *mods[4] = {"<shift _>", UNKNOWN, "<ctrl _>", "<alt _>"};

/*Wrapper for key symbol translation fucntions.
 *Parses type and selects the appropriate translation function.
 *
 *Parameters:
 *  ks_param - keystroke data
 *  buf      - output buffer
 *
 *Returns:
 *  ni nada
 */
void xlate_keysym(keystroke_data *ks_param, char *buf) {
  unsigned char type = ks_param->value >> 8;

  //high nybble doesn't seem to be useful
  type &= 0x0f;

  //select appropriate translation function
  switch (type) {
    case 0x0 : ksym_std(ks_param, buf);  break;
    case 0x1 : ksym_fnc(ks_param, buf);  break;
    case 0x2 : break;
    case 0x3 : ksym_num(ks_param, buf);  break;
    case 0x4 : break;
    case 0x5 : break;
    case 0x6 : break;
    case 0x7 : ksym_mod(ks_param, buf);  break;
    case 0x8 : break;
    case 0x9 : break;
    case 0xa : break;
    case 0xb : ksym_std(ks_param, buf);  break;
    case 0xc : break;
    case 0xd : break;
    case 0xe : break;
    case 0xf : break;
  }
}

/*Translates normal keys.
 *Index into array to get string representations of ascii characters on
 *key-press events.  For printable characters, that's just the character
 *itself; for control characters, that's an abreviation of their function.
 *Key-release events are ignored.
 *
 *Note:
 *For printable ascii characters I could have just returned the key symbol
 *value cast as a character.  However, I opted for a large table because it can
 *also handle control values, and is more easily modified to support an
 *extended ascii character set.
 */
static void ksym_std(keystroke_data *ks, char *buf) {
  unsigned char val  = ks->value & 0x00ff;

  //ignore key-release events
  if (!ks->down) return;

  //otherwise return string representation
  strlcat(buf, ascii[val], BUFLEN);
}

/*Translates f-keys and the keys typically above the arrow buttons.
 *If the high nybble of the keyboard_notifier_param's value field (ks->value)
 *is zero'd, we're dealing with an f-key; otherwise we're dealing with one of
 *the keys above the arrow pad.  F-keys are handled by inserting the low nybble
 *of the value field, incremented by 1, into a predefined string. The low
 *nybble+1 is the f-key's number. For example:
 *
 *                          +-----------------type
 *                          |      +----------val
 *                          |      |
 *  ks->value = 0xf103 = | 0xf1 | 0x03 |
 *                                 |
 *                +----------------+
 *                |
 *  buf = "<f" + val+1 + ">" = "<f4>"
 *
 *Non-f-keys are handled by using the low nybble of the value field to index
 *into an array of string represenations.
 *
 *NOTE: for testing purposes, try to get your hands on one of the keyboards in
 *      224 (19 f-keys).
 */
static void ksym_fnc(keystroke_data *ks, char *buf) {
  unsigned char val  = ks->value & 0x00ff;
  char temp[6];

  //ignore key-release events
  if (!ks->down) return;

  //non-f-keys when the high nybble isn't zero'd
  if (val & 0xf0) strlcat(buf, fncs[val&0x0f], BUFLEN);

  //f-key otherwise
  else {
    snprintf(temp, 6, "%s%d>", F_KEYS, ++val);
    strlcat(buf, temp, BUFLEN);
  }
}

/*Translates number pad keys.
 *The ledstate field of the keyboard_notifier_param must be parsed to determine
 *whether numlock is enabled or not.  See header description for more info on
 *how ledstate is handled.
 */
static void ksym_num(keystroke_data *ks, char *buf) {
  unsigned char val  = ks->value & 0x00ff;

  //just in case
  if (val > 16) return;


  //ignore key-release events
  if (!ks->down) return;

  //values depend on the state of numlock
  if (ks->ledstate & NLOCK_MASK)
    strlcat(buf, locked_npad[val], BUFLEN);
  else if (!(ks->ledstate & NLOCK_MASK))
    strlcat(buf, unlocked_npad[val], BUFLEN);
}

/*Translates normal modifier keys.
 *Return the appropriate string on both key-press and key-release events.  An
 *event-specific character is inserted into the string to indicate key pressure
 *(p for press, r for release events).
 */
static void ksym_mod(keystroke_data *ks, char *buf) {
  int len;
  unsigned char val  = ks->value & 0x00ff;

  //just in case
  if (val > 3) return;

  //translate mod key to string
  len = strlcat(buf, mods[val], BUFLEN);

  //add pressure indicator
  buf[len - 2] = ks->down ? 'p' : 'r';
}

int disp(unsigned char value) {
  return (value > 0x19 && value < 0x7F);
}
