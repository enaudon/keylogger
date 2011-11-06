#include <asm/uaccess.h>
#include <linux/fcntl.h>
#include <linux/fs.h>

typedef struct file file;

file* kopen(const char *fp, int flags, int mode);
void kclose(file *file);
int kwrite(file *file, loff_t off, char* buf, unsigned int size);

/*Opens the specified file.
 *
 *@param fp    file path
 *@param flags flags (not sure about this)
 *@param mode  permissions (this should be octal)
 */
file* kopen(const char *fp, int flags, int mode) {
  file* file = NULL;

  //allow the file system to take kernel-pointers
  mm_segment_t stored_fs = get_fs();
  set_fs(get_ds());

  //open the file
  file = filp_open(fp, flags, mode);

  //restore ok-ness of kernel-space pointers
  set_fs(stored_fs);

  return file;
}

/*Closes the specified file.
 *Notice that we can ignore the file descriptor since we keep this file
 *completely in kernel-space.  (It has no file descriptor.)
 *
 *@param file   the file to be closed
 */
void kclose(file *file) {
    filp_close(file, NULL);
}

/*Writes to the specified file.
 *
 *@param file the file to writing to
 *@param off  byte to start writing at
 *@param buf  bytes to write
 *@param size number of bytes to write
 */
int kwrite(file *file, loff_t off, char *buf, unsigned int size) {
  int ret;

  //allow the file system to take kernel-pointers
  mm_segment_t stored_fs = get_fs();
  set_fs(get_ds());

  //write to the file
  ret = vfs_write(file, buf, size, &off);

  //restore ok-ness of kernel-space pointers
  set_fs(stored_fs);

  return ret;
}

/*Reads from the specified file.
 *IMPORTANT: This method is broken.  It always seems to return -EBADF.  I don't
 *           know why.  (If I did I'd probably fix it...)
 *
 *@param file the file to reading from
 *@param off  byte to start reading from
 *@param buf  buffer to place read bytes into
 *@param size number of bytes to read
 */
int kread(file* file, loff_t off, char *buf, unsigned int size) {
  int ret;

  //allow the file system to take kernel-pointers
  mm_segment_t stored_fs = get_fs();
  set_fs(get_ds());

  //read from the file
  ret = vfs_read(file, buf, size, &off);

  //restore ok-ness of kernel-space pointers
  set_fs(stored_fs);

  return ret;
}

