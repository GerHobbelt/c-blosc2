/*********************************************************************
  Blosc - Blocked Shuffling and Compression Library

  Copyright (C) 2021  The Blosc Developers <blosc@blosc.org>
  https://blosc.org
  License: BSD 3-Clause (see LICENSE.txt)

  See LICENSE.txt for details about copyright and rights to use.
**********************************************************************/

# include <caterva.h>

int main() {

  int8_t ndim = 2;
  int64_t shape[] = {10, 10};
  int32_t chunkshape[] = {4, 4};
  int32_t blockshape[] = {2, 2};
  int32_t typesize = 8;

  int64_t slice_start[] = {2, 5};
  int64_t slice_stop[] = {2, 6};
  int32_t slice_chunkshape[] = {0, 1};
  int32_t slice_blockshape[] = {0, 1};

  int64_t nelem = 1;
  for (int i = 0; i < ndim; ++i) {
    nelem *= shape[i];
  }
  int64_t size = nelem * typesize;
  int8_t *data = malloc(size);

  blosc2_cparams cparams = BLOSC2_CPARAMS_DEFAULTS;
  cparams.typesize = typesize;

  blosc2_dparams dparams = BLOSC2_DPARAMS_DEFAULTS;
  blosc2_storage b2_storage = {.cparams=&cparams, .dparams=&dparams};

  caterva_params_t *params = caterva_new_params(&b2_storage, ndim, shape,
                                                chunkshape, blockshape, NULL, 0);

  caterva_array_t *arr;
  CATERVA_ERROR(caterva_from_buffer(data, size, params, &arr));


  blosc2_storage slice_b2_storage = {.cparams=&cparams, .dparams=&dparams};
  slice_b2_storage.urlpath = "example_hola.b2frame";
  blosc2_remove_urlpath(slice_b2_storage.urlpath);

  caterva_params_t *slice_params = caterva_new_params(&slice_b2_storage, ndim, shape,
                                                      slice_chunkshape, slice_blockshape, NULL, 0);

  caterva_array_t *slice;
  CATERVA_ERROR(caterva_get_slice(arr, slice_start, slice_stop, slice_params, &slice));

  uint8_t *buffer;
  uint64_t buffer_size = 1;
  for (int i = 0; i < slice->ndim; ++i) {
    buffer_size *= slice->shape[i];
  }
  buffer_size *= slice->sc->typesize;
  buffer = malloc(buffer_size);

  CATERVA_ERROR(caterva_to_buffer(slice, buffer, buffer_size));

  // printf("Elapsed seconds: %.5f\n", blosc_elapsed_secs(t0, t1));
  CATERVA_ERROR(caterva_free_params(params));

  return 0;
}
