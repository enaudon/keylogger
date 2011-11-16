#include <linux/keyboard.h>

typedef struct keyboard_notifier_param keystroke_data;

static void ksym_std(keystroke_data *ks, char *buf);
static void ksym_mod(keystroke_data *ks, char *buf);

//key symbol to string tables
char *mods[4] = {"<shift _>", "", "<ctrl _>", "<alt _>"};
//char *fn[

/*Wrapper for key symbol translation fucntions.
 *Parses value and selects the appropriate translation function.
 *
 *@param sym  keysymbol to translate
 *@param flag true = depressed
 *@param buf  output buffer
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
    case 0x3 :
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

static void ksym_std(keystroke_data *ks, char *buf) {
  if (ks->down) sprintf(buf, "%c", ks->value);
  else          buf[0] = 0x00;
}

static void ksym_mod(keystroke_data *ks, char *buf) {
  int len;

  //grab value and presure flag from keystroke data struct
  unsigned char val  = ks->value & 0x00ff;
  int flag = ks->down;

  //just in case
  if (val > 4) return;

  //translate mod key to string
  len = sprintf(buf, "%s", mods[val]);

  //add pressure indicator
  buf[len - 2] = flag ? 'p' : 'r';
}

int disp(unsigned char value) {
  return (value > 0x19 && value < 0x7F);
}
