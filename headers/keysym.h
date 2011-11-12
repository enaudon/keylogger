static void ksym_std(unsigned char value, int flag, char *buf);
static void ksym_mod(unsigned char value, int flag, char *buf);

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
void xlate_keysym(unsigned short sym, int flag, char *buf) {

  //parse key symbol
  //      <----sym---->
  // ...|  typ  |  val  |...
  unsigned char type = sym >> 8;
  unsigned char val  = sym & 0x00ff;

  //select appropriate translation function
  switch (type & 0x0f) {
    case 0x0 : 
    case 0x1 :
    case 0x2 :
    case 0x3 :
    case 0x4 :
    case 0x5 :
    case 0x6 :
    case 0x7 : ksym_mod(val, flag, buf);  break;
    case 0x8 :
    case 0x9 :
    case 0xa :
    case 0xb : ksym_std(val, flag, buf);  break;
    case 0xc :
    case 0xd :
    case 0xe :
    case 0xf : break;
  }
}

static void ksym_std(unsigned char val, int flag, char *buf) {
  if (flag) sprintf(buf, "%c", val);
  else      sprintf(buf, "");
}

static void ksym_mod(unsigned char val, int flag, char *buf) {

  //just in case
  if (val > 4) return;

  //translate mod key to string
  int len = sprintf(buf, "%s", mods[val]);

  //add pressure indicator
  buf[len - 2] = flag ? 'p' : 'r';
}

int disp(unsigned char value) {
  return (value > 0x19 && value < 0x7F);
}
