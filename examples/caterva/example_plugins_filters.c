/*********************************************************************
  Blosc - Blocked Shuffling and Compression Library

  Copyright (C) 2021  The Blosc Developers <blosc@blosc.org>
  https://blosc.org
  License: BSD 3-Clause (see LICENSE.txt)

  See LICENSE.txt for details about copyright and rights to use.
**********************************************************************/

/* Example program demonstrating use of the Blosc plugins from C code.
*
* To compile this program:
* $ gcc example_plugins_filters.c -o example_plugins_filters -lblosc2
*
* To run:
* $ ./example_plugins_filters
*
* from_buffer: 0.0668 s
* to_buffer: 0.0068 s
* Process finished with exit code 0
*/



#include <caterva.h>
#include <stdio.h>
#include <blosc2.h>
#include "../../plugins/filters/filters-registry.c"
#include <inttypes.h>

int main() {
  blosc_timestamp_t t0, t1;

  blosc2_init();
  int8_t ndim = 3;
  int32_t typesize = sizeof(int64_t);

  int64_t shape[] = {345, 200, 50};
  int32_t chunkshape[] = {150, 100, 50};
  int32_t blockshape[] = {21, 30, 27};

  int64_t nbytes = typesize;
  for (int i = 0; i < ndim; ++i) {
    nbytes *= shape[i];
  }

  int64_t *src = malloc((size_t) nbytes);
  for (int i = 0; i < nbytes / typesize; ++i) {
    src[i] = (int64_t) i;
  }

  blosc2_cparams cparams = BLOSC2_CPARAMS_DEFAULTS;
  cparams.nthreads = 1;
  /*
   * Use the NDCELL filter through its plugin.
   * NDCELL metainformation: user must specify the parameter meta as the cellshape, so
   * if in a 3-dim dataset user specifies meta = 4, then cellshape will be 4x4x4.
  */
  cparams.filters[4] = BLOSC_FILTER_NDCELL;
  cparams.filters_meta[4] = 4;
  cparams.typesize = typesize;
  // We could use a codec plugin by setting cparams.compcodec.

  blosc2_dparams dparams = BLOSC2_DPARAMS_DEFAULTS;
  blosc2_storage b2_storage = {.cparams=&cparams, .dparams=&dparams};

  caterva_params_t *params = caterva_new_params(&b2_storage, ndim, shape, chunkshape, blockshape,
                                                NULL, 0);

  caterva_array_t *arr;
  blosc_set_timestamp(&t0);
  CATERVA_ERROR(caterva_from_buffer(src, nbytes, params, &arr));
  blosc_set_timestamp(&t1);
  printf("from_buffer: %.4f s\n", blosc_elapsed_secs(t0, t1));

  int64_t *buffer = malloc(nbytes);
  int64_t buffer_size = nbytes;
  blosc_set_timestamp(&t0);
  CATERVA_ERROR(caterva_to_buffer(arr, buffer, buffer_size));
  blosc_set_timestamp(&t1);
  printf("to_buffer: %.4f s\n", blosc_elapsed_secs(t0, t1));

  blosc2_destroy();

  for (int i = 0; i < buffer_size / typesize; i++) {
    if (src[i] != buffer[i]) {
      printf("\n Decompressed data differs from original!\n");
      printf("i: %d, data %" PRId64 ", dest %" PRId64 "", i, src[i], buffer[i]);
      return -1;
    }
  }

  free(src);
  free(buffer);

  caterva_free(&arr);
  CATERVA_ERROR(caterva_free_params(params));

  return 0;
}
