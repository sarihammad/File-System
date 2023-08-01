/**
 * vsfs command line options parser header file.
 */

#pragma once

#include <stdbool.h>

#include <fuse_opt.h>

/** vsfs command line options. */
typedef struct vsfs_opts
{
  /** vsfs image file path. */
  const char* img_path;
  /** Print help and exit. FUSE option. */
  int help;

} vsfs_opts;

/**
 * Parse vsfs command line options.
 *
 * @param args  pointer to 'struct fuse_args' with the program arguments.
 * @param args  pointer to the options struct that receives the result.
 * @return      true on success; false on failure.
 */
bool
vsfs_opt_parse(struct fuse_args* args, vsfs_opts* opts);
