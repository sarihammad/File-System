/**
 * bitmap utility functions.
 */

#pragma once

#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "util.h"

typedef size_t bitmap_t;

// Initialize the first nbits bits of bitmap to 0 (meaning available).
int
bitmap_init(bitmap_t* b, uint32_t nbits);

// Find the first unused bit in bitmap b and return the index of the bit in
// *index. Returns 0 on success and -1 if all bits are already marked as in-use.
int
bitmap_alloc(bitmap_t* b, uint32_t nbits, uint32_t* index);

// Marks the bit at the given index as available (0).
// The supplied index must be less than the number of bits in the bitmap.
// The bitmap at the supplied index must be marked allocated.
void
bitmap_free(bitmap_t* b, uint32_t nbits, uint32_t index);

// Set the bit at index to 0 if val==false, or 1 if val == true
void
bitmap_set(bitmap_t* b, uint32_t nbits, uint32_t index, bool val);

// Returns true is the bit at index is set to 1, otherwise false
bool
bitmap_isset(bitmap_t* b, uint32_t nbits, uint32_t index);
