
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <libgen.h>

// Using 2.9.x FUSE API
#define FUSE_USE_VERSION 29
#include <fuse.h>

#include "bitmap.h"
#include "fs_ctx.h"
#include "map.h"
#include "options.h"
#include "util.h"
#include "vsfs.h"


/**
 * Initialize the file system.
 *
 * Called when the file system is mounted. NOTE: we are not using the FUSE
 * init() callback since it doesn't support returning errors. This function must
 * be called explicitly before fuse_main().
 *
 * @param fs    file system context to initialize.
 * @param opts  command line options.
 * @return      true on success; false on failure.
 */
static bool
vsfs_init(fs_ctx* fs, vsfs_opts* opts)
{
  size_t size;
  void* image;

  // Nothing to initialize if only printing help
  if (opts->help) {
    return true;
  }

  // Map the disk image file into memory
  image = map_file(opts->img_path, VSFS_BLOCK_SIZE, &size);
  if (image == NULL) {
    return false;
  }

  return fs_ctx_init(fs, image, size);
}

/**
 * Cleanup the file system.
 *
 * Called when the file system is unmounted.
 */
static void
vsfs_destroy(void* ctx)
{
  fs_ctx* fs = (fs_ctx*)ctx;
  if (fs->image) {
    munmap(fs->image, fs->size);
    fs_ctx_destroy(fs);
  }
}

/** Get file system context. */
static fs_ctx*
get_fs(void)
{
  return (fs_ctx*)fuse_get_context()->private_data;
}

static void *get_address(vsfs_inode *ino, uint32_t offset) {
  fs_ctx* fs = get_fs();
	vsfs_blk_t *blocks;
	uint32_t block_num = offset / VSFS_BLOCK_SIZE;
	if (block_num >= VSFS_NUM_DIRECT) {
    block_num -= VSFS_NUM_DIRECT;
    blocks = fs->image + (ino->i_indirect) * VSFS_BLOCK_SIZE;

	} else {
    blocks = ino->i_direct;
	}

	return fs->image + blocks[block_num] * VSFS_BLOCK_SIZE + (offset % VSFS_BLOCK_SIZE);
}


static vsfs_dentry *get_dir_entry(vsfs_inode *ino, uint64_t i) {
  vsfs_dentry *dentry = (vsfs_dentry *) get_address(ino, i * sizeof(vsfs_dentry));
  return dentry;
}





/* Returns the inode number for the element at the end of the path
 * if it exists.  If there is any error, return -1.
 * Possible errors include:
 *   - The path is not an absolute path
 *   - An element on the path cannot be found
 */
static int
path_lookup(const char* path, vsfs_ino_t* ino)
{
  if (path[0] != '/') {
    fprintf(stderr, "Not an absolute path\n");
    return -1;
  }

  if (strcmp(path, "/") == 0) {
    *ino = VSFS_ROOT_INO;
    return 0;
  }

  vsfs_dentry *dir_entry;
  for (uint64_t i = 0; i < VSFS_BLOCK_SIZE / sizeof(vsfs_dentry); i++) {
    dir_entry = get_dir_entry(&get_fs()->itable[0], i);
    if (dir_entry->ino != VSFS_INO_MAX && strcmp((const char *) (path + 1), dir_entry->name) == 0) {
      *ino = dir_entry->ino;
	    return 0;
    }
  }

  *ino = VSFS_INO_MAX;
	return -1;
}

/**
 * Get file system statistics.
 *
 * Implements the statvfs() system call.
 *
 * Errors: none
 *
 * @param path  path to any file in the file system.
 * @param st    pointer to the struct statvfs that receives the result.
 * @return      0 on success; -errno on error.
 */
static int
vsfs_statfs(const char* path, struct statvfs* st)
{
  (void)path; // unused
  fs_ctx* fs = get_fs();
  vsfs_superblock* sb = fs->sb; /* Get ptr to superblock from context */

  memset(st, 0, sizeof(*st));
  st->f_bsize = VSFS_BLOCK_SIZE;  /* Filesystem block size */
  st->f_frsize = VSFS_BLOCK_SIZE; /* Fragment size */
  st->f_blocks = sb->num_blocks;  /* Size of fs in f_frsize units */
  st->f_bfree = sb->free_blocks;  /* Number of free blocks */
  st->f_bavail = sb->free_blocks; /* Free blocks for unpriv users */
  st->f_files = sb->num_inodes;   /* Number of inodes */
  st->f_ffree = sb->free_inodes;  /* Number of free inodes */
  st->f_favail = sb->free_inodes; /* Free inodes for unpriv users */

  st->f_namemax = VSFS_NAME_MAX; /* Maximum filename length */

  return 0;
}


// static char*
// get_next_comp(char* p)
// {
//     int c = *p;
//     while (c) {
//         if (c == '/') { return p; }
//         p++;
//         c = *p;
//     }
//     return 0;
// }

/**
 * Get file or directory attributes.
 *
 * Implements the lstat() system call.
 * The following fields can be ignored: st_dev, st_ino, st_uid, st_gid, st_rdev,
 *                                      st_blksize, st_atim, st_ctim.
 *
 * Errors:
 *   ENAMETOOLONG  the path or one of its components is too long.
 *   ENOENT        a component of the path does not exist.
 *   ENOTDIR       a component of the path prefix is not a directory.
 *
 * @param path  path to a file or directory.
 * @param st    pointer to the struct stat that receives the result.
 * @return      0 on success; -errno on error;
 */
static int
vsfs_getattr(const char* path, struct stat* st)
{
  if (strlen(path) >= VSFS_PATH_MAX || strlen(path) >= VSFS_NAME_MAX + 1)
    return -ENAMETOOLONG;
  fs_ctx* fs = get_fs();

  memset(st, 0, sizeof(*st));

  
  // Error checking assuming longer paths

  /*
  const char *start = path;
  char *end;
  char curr_path[VSFS_PATH_MAX];

  if (*start == '/') { start++; }

  while ((end = get_next_comp(start))) {

    size_t comp_len = end - start;
    if (comp_len >= VSFS_NAME_MAX) return -ENAMETOOLONG;

    size_t n = end - path;
    memcpy(curr_path, path, n);
    curr_path[n] = '\0';

    vsfs_ino_t ino;
    int err = path_lookup(curr_path, &ino);
    if (err == -1) return -ENOENT;
    if (!S_ISDIR(fs->itable[ino].i_mode)) return -ENOTDIR;
    start = end + 1;
  } */

  vsfs_ino_t ino;
  int err = path_lookup(path, &ino);
  if (err == -1) return -ENOENT;
    
  st->st_mode = fs->itable[ino].i_mode;
  st->st_nlink = fs->itable[ino].i_nlink;
  st->st_size = fs->itable[ino].i_size;
  st->st_blocks = div_round_up(fs->itable[ino].i_size, 512);
  st->st_mtim = fs->itable[ino].i_mtime;

  return 0;
}

/**
 * Read a directory.
 *
 * Implements the readdir() system call. 
 * 
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" exists and is a directory.
 *
 * Errors:
 *   ENOMEM  not enough memory (e.g. a filler() call failed).
 *
 * @param path    path to the directory.
 * @param buf     buffer that receives the result.
 * @param filler  function that needs to be called for each directory entry.
 *                Pass 0 as offset (4th argument). 3rd argument can be NULL.
 * @param offset  unused.
 * @param fi      unused.
 * @return        0 on success; -errno on error.
 */
static int
vsfs_readdir(const char* path,
             void* buf,
             fuse_fill_dir_t filler,
             off_t offset,
             struct fuse_file_info* fi)
{
  (void)offset; // unused
  (void)fi;     // unused
  fs_ctx* fs = get_fs();

  assert(strcmp(path, "/") == 0);
  for (uint64_t i = 0; i < VSFS_BLOCK_SIZE / sizeof(vsfs_dentry); i++) {
    vsfs_dentry *dir_entry = get_dir_entry(&fs->itable[0], i);
    if (dir_entry->ino != VSFS_INO_MAX) {
      int is_full = filler(buf, dir_entry->name, NULL, 0);
      if (is_full) { return -ENOMEM; }
    }
  }
  return 0;
}


/**
 * Create a file.
 *
 * Implements the open()/creat() system call.
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" doesn't exist.
 *   The parent directory of "path" exists and is a directory.
 *   "path" and its components are not too long.
 *
 * Errors:
 *   ENOMEM  not enough memory (e.g. a malloc() call failed).
 *   ENOSPC  not enough free space in the file system.
 *
 * @param path  path to the file to create.
 * @param mode  file mode bits.
 * @param fi    unused.
 * @return      0 on success; -errno on error.
 */
static int
vsfs_create(const char* path, mode_t mode, struct fuse_file_info* fi)
{
  (void)fi; // unused
  assert(S_ISREG(mode));
  fs_ctx* fs = get_fs();

  vsfs_superblock *sb = (vsfs_superblock *) fs->image;
  
  if (sb->free_inodes == 0) { return -ENOSPC; }

  // Find available inode
  uint32_t new_ino;
  if (bitmap_alloc(fs->ibmap, sb->num_inodes, &new_ino) < 0) return -ENOSPC;
  bitmap_set(fs->ibmap, sb->num_inodes, new_ino, true);
  vsfs_inode *new_inode = &(fs->itable[new_ino]);
  memset(new_inode, 0, sizeof(vsfs_inode));
  sb->free_inodes--;

  // Create new inode
  new_inode->i_size = 0;
  new_inode->i_blocks = 0;
  new_inode->i_mode = mode;
  new_inode->i_nlink = 1;
  clock_gettime(CLOCK_REALTIME, &(new_inode->i_mtime));

  // Add inode to parent dentry
  for (uint64_t i = 0; i < VSFS_BLOCK_SIZE / sizeof(vsfs_dentry); i++) {
    vsfs_dentry *dir_entry = get_dir_entry(&fs->itable[0], i);
    if (dir_entry->ino == VSFS_INO_MAX) {
      dir_entry->ino = new_ino;
      strcpy(dir_entry->name, strrchr(path, '/') + 1);
      vsfs_inode* parent_inode = &(fs->itable[0]);
      clock_gettime(CLOCK_REALTIME, &(parent_inode->i_mtime));
      return 0;
    }
  }
  return -ENOSPC;
}

/**
 * Remove a file.
 *
 * Implements the unlink() system call.
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" exists and is a file.
 *
 * Errors: none
 *
 * @param path  path to the file to remove.
 * @return      0 on success; -errno on error.
 */
static int
vsfs_unlink(const char* path)
{
  fs_ctx* fs = get_fs();


  // Get file inode from path
  vsfs_ino_t ino;
  path_lookup(path, &ino);
  vsfs_inode *inode = &(fs->itable[ino]);

  inode->i_nlink--;

  // Free the inode
  if (inode->i_nlink == 0) {
    bitmap_free(fs->ibmap, fs->sb->num_inodes, (uint32_t) ino);
    fs->sb->free_inodes++;
  }

  // Free the blocks
  for (uint32_t i = 0; i < inode->i_blocks; i++) {
    bitmap_free(fs->dbmap, fs->sb->num_blocks, inode->i_direct[i]);
    fs->sb->free_blocks++;
  }

  // Find the dir entry, set its ino to max and name to nothing
  for (uint64_t i = 0; i < VSFS_BLOCK_SIZE / sizeof(vsfs_dentry); i++) {
    vsfs_dentry *dir_entry = get_dir_entry(&fs->itable[0], i);
    if (dir_entry->ino != VSFS_INO_MAX && strcmp((const char *) (path + 1), dir_entry->name) == 0) {
      memset(dir_entry, 0, strlen(dir_entry->name));
      dir_entry->ino = VSFS_INO_MAX;
      vsfs_inode* parent_inode = &(fs->itable[0]);
      clock_gettime(CLOCK_REALTIME, &(parent_inode->i_mtime));
      return 0;
    }
  }

  return 0;


}

/**
 * Change the modification time of a file or directory.
 *
 * Implements the utimensat() system call.
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" exists.
 *
 * Errors: none
 *
 * @param path   path to the file or directory.
 * @param times  timestamps array. See "man 2 utimensat" for details.
 * @return       0 on success; -errno on failure.
 */
static int
vsfs_utimens(const char* path, const struct timespec times[2])
{
  fs_ctx* fs = get_fs();
  vsfs_inode* ino = NULL;

  (void)path;
  (void)fs;
  (void)ino;

  // 0. Check if there is actually anything to be done.
  if (times[1].tv_nsec == UTIME_OMIT) {
    // Nothing to do.
    return 0;
  }

  // Find the inode for the final component in path
  vsfs_ino_t ino_num;
	path_lookup(path, &ino_num);
	ino = &fs->itable[ino_num];

  // Update the mtime for that inode.
  if (times[1].tv_nsec == UTIME_NOW) {
    if (clock_gettime(CLOCK_REALTIME, &(ino->i_mtime)) != 0) {
    	assert(false);
    }
  } else {
    ino->i_mtime = times[1];
  }

  return 0;
}

/**
 * Change the size of a file.
 *
 * Implements the truncate() system call. Supports both extending and shrinking.
 * If the file is extended, the new uninitialized range at the end is 
 * filled with zeros.
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" exists and is a file.
 *
 * Errors:
 *   ENOMEM  not enough memory (e.g. a malloc() call failed).
 *   ENOSPC  not enough free space in the file system.
 *   EFBIG   write would exceed the maximum file size.
 *
 * @param path  path to the file to set the size.
 * @param size  new file size in bytes.
 * @return      0 on success; -errno on error.
 */
static int
vsfs_truncate(const char* path, off_t size)
{
  fs_ctx* fs = get_fs();

  (void)path;
  (void)size;
  (void)fs;

  vsfs_blk_t block_size = div_round_up(size, VSFS_BLOCK_SIZE);

  if (block_size > (VSFS_NUM_DIRECT) + (VSFS_BLOCK_SIZE / sizeof(vsfs_blk_t))) return -EFBIG;

  vsfs_ino_t ino_num;
	path_lookup(path, &ino_num);
	vsfs_inode *ino = &fs->itable[ino_num];

  if ((uint64_t) size == ino->i_size) return 0;

  // zero out the uninitialized range
  if ((uint64_t) size > ino->i_size) {
    memset(get_address(ino, ino->i_size), 0, size - ino->i_size);
  }

  if (block_size > ino->i_blocks) { // allocate more blocks
    uint32_t num_to_allocate = (block_size - ino->i_blocks) > VSFS_NUM_DIRECT ? VSFS_NUM_DIRECT : (block_size - ino->i_blocks);
    for (uint32_t i = 0; i < num_to_allocate; i++) {
      uint32_t block_num;
      if (bitmap_alloc(fs->dbmap, fs->sb->num_blocks, &block_num) < 0) return -ENOSPC;
      bitmap_set(fs->dbmap, fs->sb->num_blocks, block_num, true);
      fs->sb->free_blocks--;
    }
  } else { // free blocks
    // uint32_t block_i = fs->sb->num_blocks - 1;
    uint32_t num_to_free = (ino->i_blocks - block_size) > VSFS_NUM_DIRECT ? VSFS_NUM_DIRECT : (ino->i_blocks - block_size);
    for (uint32_t i = 0; i < num_to_free; i++) {
      // while (!bitmap_isset(fs->dbmap, fs->sb->num_blocks, block_i)) block_i--;
      bitmap_free(fs->dbmap, fs->sb->num_blocks, ino->i_direct[VSFS_NUM_DIRECT - 1 - i]);
      fs->sb->free_blocks++;
    }
  }

  // Set new file size
  ino->i_size = size;
  ino->i_blocks = block_size;
 
  clock_gettime(CLOCK_REALTIME, &(ino->i_mtime));

  return 0;

}

/**
 * Read data from a file.
 *
 * Implements the pread() system call. returns exactly the number of bytes
 * requested except on EOF (end of file). Reads from file ranges that have not
 * been written to return ranges filled with zeros.
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" exists and is a file.
 *   the byte range from offset to offset + size is contained within a single block.
 *
 * Errors: none
 *
 * @param path    path to the file to read from.
 * @param buf     pointer to the buffer that receives the data.
 * @param size    buffer size (number of bytes requested).
 * @param offset  offset from the beginning of the file to read from.
 * @param fi      unused.
 * @return        number of bytes read on success; 0 if offset is beyond EOF;
 *                -errno on error.
 */
static int
vsfs_read(const char* path,
          char* buf,
          size_t size,
          off_t offset,
          struct fuse_file_info* fi)
{
  (void)fi; // unused
  fs_ctx* fs = get_fs();

  vsfs_ino_t ino;
  path_lookup(path, &ino);
  vsfs_inode *inode = &(fs->itable[ino]);

  if (inode->i_size < (uint64_t) offset) { return 0; }

  if (inode->i_size < (uint64_t) offset + (uint64_t) size) { size = inode->i_size - offset; }

  memcpy(buf, (char *) get_address(inode, offset), size);

  return (int) size;
  
}

/**
 * Write data to a file.
 *
 * Implements the pwrite() system call. returns exactly the number of bytes
 * requested except on error. If the offset is beyond EOF (end of file), the
 * file is extended. If the write creates a hole of uninitialized data,
 * the new uninitialized range is filled with zeros.
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" exists and is a file.
 *   the byte range from offset to offset + size is contained within a single block.
 *
 * Errors:
 *   ENOMEM  not enough memory (e.g. a malloc() call failed).
 *   ENOSPC  not enough free space in the file system.
 *   EFBIG   write would exceed the maximum file size
 *
 * @param path    path to the file to write to.
 * @param buf     pointer to the buffer containing the data.
 * @param size    buffer size (number of bytes requested).
 * @param offset  offset from the beginning of the file to write to.
 * @param fi      unused.
 * @return        number of bytes written on success; -errno on error.
 */
static int
vsfs_write(const char* path,
           const char* buf,
           size_t size,
           off_t offset,
           struct fuse_file_info* fi)
{
  (void)fi; // unused
  fs_ctx* fs = get_fs();


  // get inode
  vsfs_ino_t ino;
  path_lookup(path, &ino);
  vsfs_inode *inode = &(fs->itable[ino]);

  if (inode->i_size < (uint64_t) offset) return -EFBIG;
  
  // truncate if needed
  if (inode->i_size < (uint64_t) offset + (uint64_t) size) {
    int res = vsfs_truncate(path, offset + size);
    if (res < 0) return res;
  } else {
    // update inode
    inode->i_size += size;
    inode->i_blocks += div_round_up(size, VSFS_BLOCK_SIZE);
  }

  // do the write to the data block
  memcpy((char *) get_address(inode, offset), buf, size);

  // update last modified time
  clock_gettime(CLOCK_REALTIME, &(inode->i_mtime));

  return (int) size;
}

static struct fuse_operations vsfs_ops = {
  .destroy = vsfs_destroy,
  .statfs = vsfs_statfs,
  .getattr = vsfs_getattr,
  .readdir = vsfs_readdir,
  .mkdir = vsfs_mkdir,
  .rmdir = vsfs_rmdir,
  .create = vsfs_create,
  .unlink = vsfs_unlink,
  .utimens = vsfs_utimens,
  .truncate = vsfs_truncate,
  .read = vsfs_read,
  .write = vsfs_write,
};

int
main(int argc, char* argv[])
{
  vsfs_opts opts = { 0 }; // defaults are all 0
  struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
  if (!vsfs_opt_parse(&args, &opts))
    return 1;

  fs_ctx fs = { 0 };
  if (!vsfs_init(&fs, &opts)) {
    fprintf(stderr, "Failed to mount the file system\n");
    return 1;
  }

  return fuse_main(args.argc, args.argv, &vsfs_ops, &fs);
}
