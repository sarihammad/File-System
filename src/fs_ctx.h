/**
 * File system runtime context header file.
 */

#pragma once

//#include <stdlib.h>
#include <stddef.h>
//#include <unistd.h>
//#include <sys/types.h>
#include "bitmap.h"
#include "options.h"
#include "vsfs.h"

/**
 * Mounted file system runtime state - "fs context".
 */
typedef struct fs_ctx
{
  /** Pointer to the start of the image. */
  void* image;
  /** Image size in bytes. */
  size_t size;
  /** Pointer to the superblock in the mmap'd disk image */
  vsfs_superblock* sb;
  /** Pointer to the inode bitmap in the mmap'd disk image */
  bitmap_t* ibmap;
  /** Pointer to the data block bitmap in the mmap'd disk image */
  bitmap_t* dbmap;
  /** Pointer to the inode table in the mmap'd disk image */
  vsfs_inode* itable;

  // TODO: other useful runtime state of the mounted file system should be
  //       cached here (NOT in global variables in vsfs.c)
  int error_code;

} fs_ctx;

/**
 * Initialize file system context.
 *
 * @param fs     pointer to the context to initialize.
 * @param size   image size in bytes.
 * @return       true on success; false on failure (e.g. invalid superblock).
 */
bool
fs_ctx_init(fs_ctx* fs, void* image, size_t size);

/**
 * Destroy file system context.
 * Must cleanup all the resources created in fs_ctx_init().
 *
 * @param fs     pointer to the context to clean up
 */
void
fs_ctx_destroy(fs_ctx* fs);
