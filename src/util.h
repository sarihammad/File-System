/**
 * Miscellaneous utility functions.
 */

#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

/** Check if x is a power of 2. */
static inline bool
is_powerof2(size_t x)
{
  return (x & (x - 1)) == 0;
}

/** Check if x is a multiple of alignment (which must be a power of 2). */
static inline bool
is_aligned(size_t x, size_t alignment)
{
  assert(is_powerof2(alignment));
  return (x & (alignment - 1)) == 0;
}

/** Align x up to a multiple of alignment (which must be a power of 2). */
static inline size_t
align_up(size_t x, size_t alignment)
{
  assert(is_powerof2(alignment));
  return (x + alignment - 1) & (~alignment + 1);
}

/** Return the integer ceiling of x / y. */
static inline uint32_t
div_round_up(uint32_t x, uint32_t y)
{
  return (x + y - 1) / y;
}
