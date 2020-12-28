/*********************************************************************
  Blosc - Blocked Shuffling and Compression Library

  Author: The Blosc Developers <blosc@blosc.org>
  Creation date: 2017-08-29

  See LICENSE.txt for details about copyright and rights to use.
**********************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include "btune.h"


/* Whether a codec is meant for High Compression Ratios
   Includes LZ4 + BITSHUFFLE here, but not BloscLZ + BITSHUFFLE because,
   BloscLZ always works with splits, which have a different blocksize computation. */
static bool is_HCR(blosc2_context * context) {
  switch (context->compcode) {
    case BLOSC_BLOSCLZ :
      return false;
    case BLOSC_LZ4 :
      return (context->filter_flags & BLOSC_DOBITSHUFFLE) ? true : false;
    case BLOSC_LZ4HC :
      return true;
    case BLOSC_LIZARD :
      return true;
    case BLOSC_ZLIB :
      return true;
    case BLOSC_ZSTD :
      return true;
    case BLOSC_NDLZ :
      return false;
    default :
      BLOSC_TRACE_ERROR("Error in is_COMP_HCR: codec %d not handled.",
              context->compcode);
  }
  return false;
}

// Set the automatic blocksize 0 to its real value
void btune_next_blocksize(blosc2_context *context) {
  int32_t clevel = context->clevel;
  int32_t typesize = context->typesize;
  int32_t nbytes = context->sourcesize;
  int32_t user_blocksize = context->blocksize;
  int32_t blocksize = nbytes;

  // Protection against very small buffers
  if (nbytes < typesize) {
    context->blocksize = 1;
    return;
  }

  if (user_blocksize) {
    blocksize = user_blocksize;
    goto last;
  }

  if (nbytes >= L1) {
    blocksize = L1;

    /* For HCR codecs, increase the block sizes by a factor of 2 because they
        are meant for compressing large blocks (i.e. they show a big overhead
        when compressing small ones). */
    if (is_HCR(context)) {
      blocksize *= 2;
    }

    // Choose a different blocksize depending on the compression level
    switch (clevel) {
      case 0:
        // Case of plain copy
        blocksize /= 4;
        break;
      case 1:
        blocksize /= 2;
        break;
      case 2:
        blocksize *= 1;
        break;
      case 3:
        blocksize *= 2;
        break;
      case 4:
      case 5:
        blocksize *= 4;
        break;
      case 6:
      case 7:
      case 8:
        blocksize *= 8;
        break;
      case 9:
        // Do not exceed 256 KB for non HCR codecs
        blocksize *= 8;
        if (is_HCR(context)) {
          blocksize *= 2;
        }
        break;
      default:
        break;
    }
  }

  /* Now the blocksize for splittable codecs */
  if (clevel > 0 && split_block(context, typesize, blocksize, true)) {
    // For performance reasons, do not exceed 256 KB (in must fit in L2 cache)
    switch (clevel) {
      case 1:
        blocksize = 8 * 1024;
        break;
      case 2:
      case 3:
        blocksize = 16 * 1024;
        break;
      case 4:
      case 5:
      case 6:
      case 7:
      case 8:
        blocksize = 128 * 1024;
        break;
      case 9:
      default:
        blocksize = 256 * 1024;
        break;
    }
    // Multiply by typesize so as to get proper split sizes
    blocksize *= typesize;
    // But do not exceed 1 MB per thread (having this capacity in L3 is normal in modern CPUs)
    if (blocksize > 1024 * 1024) {
      blocksize = 1024 * 1024;
    }
    if (blocksize < 32 * 1024) {
      /* Do not use a too small blocksize (< 32 KB) when typesize is small */
      blocksize = 32 * 1024;
    }
  }

  last:
  /* Check that blocksize is not too large */
  if (blocksize > nbytes) {
    blocksize = nbytes;
  }

  // blocksize *must absolutely* be a multiple of the typesize
  if (blocksize > typesize) {
    blocksize = blocksize / typesize * typesize;
  }

  context->blocksize = blocksize;
}

void btune_next_cparams(blosc2_context * context) {
    BLOSC_UNUSED_PARAM(context);
}

void btune_update(blosc2_context * context, double ctime) {
    BLOSC_UNUSED_PARAM(context);
    BLOSC_UNUSED_PARAM(ctime);
}

void btune_free(blosc2_context * context) {
    BLOSC_UNUSED_PARAM(context);
}
