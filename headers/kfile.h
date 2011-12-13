#include <asm/uaccess.h>
#include <linux/fcntl.h>
#include <linux/fs.h>

typedef struct file file;

file* kopen(const char *fp, int flags, int mode);
int kread(file* file, loff_t off, char *buf, unsigned int size);
int kwrite(file *file, loff_t off, char* buf, unsigned int size);
void kclose(file *file);

/*Allow the file system to take kernel pointers.
 *
 *Note:
 *This one could do with a better name methinks.
 */
#define MODIFY_FS \
  mm_segment_t old_fs = get_fs();\
  set_fs(get_ds());

/*Restore the pointer range that the fs will accept.
 */
#define RESTORE_FS \
  set_fs(old_fs);

/*Opens the specified file.
 *
 *Parameters:
 *  fp    - file path to be opened
 *  flags - flags dictating file usage (read, write, exec, etc.)
 *  mode  - permissions (this should be octal)
 *
 *Returns:
 *  pointer to file struct
 */
file* kopen(const char *fp, int flags, int mode) {
  file *file = NULL;

  //open the file
  MODIFY_FS
  file = filp_open(fp, flags, mode);
  RESTORE_FS

  return file;
}

/*Reads from the specified file.
 *
 *Parameters:
 *  file - the file to reading from
 *  off  - byte to start reading from
 *  buf  - buffer to place read bytes into
 *  size - number of bytes to read
 *
 *Returns:
 *  number of bytes read
 */
int kread(file* file, loff_t off, char *buf, unsigned int size) {
  int ret;

  //read from the file
  MODIFY_FS
  ret = vfs_read(file, buf, size, &off);
  RESTORE_FS

  return ret;
}

/*Writes to the specified file.
 *
 *Parameters:
 *  file - the file to writing to
 *  off  - byte to start writing at
 *  buf  - bytes to write
 *  size - number of bytes to write
 *
 *Returns:
 *  number of bytes written
 */
int kwrite(file *file, loff_t off, char *buf, unsigned int size) {
  int ret;

  //write to the file
  MODIFY_FS
  ret = vfs_write(file, buf, size, &off);
  RESTORE_FS

  return ret;
}

/*Closes the specified file.
 *Notice that we can ignore the file descriptor since we keep this file
 *completely in kernel-space.  (It has no file descriptor.)
 *
 *Parameter
 *  file - the file to be closed
 */
void kclose(file *file) {
  filp_close(file, NULL);
}

/*
 *These methods should be implemented at some point.  (They provide good
 *encapsulation.)  Although, the KERNEL_DS part should be changed to get_ds(),
 *as above.  (More general applicability that way.)

static mm_segment_t old_fs;

static inline void begin_kmem(void){
  old_fs = get_fs();
  set_fs(KERNEL_DS);
}

static inline void end_kmem(void){
  set_fs(old_fs);
}
*/

