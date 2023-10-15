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
    int8_t typesize = 8;

    int64_t nelem = 1;
    for (int i = 0; i < ndim; ++i) {
        nelem *= shape[i];
    }
    int64_t size = nelem * typesize;
    double *data = malloc(size);

    for (int i = 0; i < nelem; ++i) {
        data[i] = i;
    }

    blosc2_cparams cparams = BLOSC2_CPARAMS_DEFAULTS;
    cparams.typesize = typesize;
    blosc2_context *ctx = blosc2_create_cctx(cparams);

    caterva_params_t params = {0};
    params.ndim = ndim;
    for (int i = 0; i < ndim; ++i) {
        params.shape[i] = shape[i];
    }

    blosc2_dparams dparams = BLOSC2_DPARAMS_DEFAULTS;
    blosc2_storage b_storage = {.cparams=&cparams, .dparams=&dparams};
    caterva_storage_t storage = {.b_storage=&b_storage};
    int32_t blocknitems = 1;
    for (int i = 0; i < ndim; ++i) {
        storage.chunkshape[i] = chunkshape[i];
        storage.blockshape[i] = blockshape[i];
        blocknitems *= storage.blockshape[i];
    }
    storage.b_storage->cparams->blocksize = blocknitems * storage.b_storage->cparams->typesize;
    storage.b_storage->contiguous = false;

    caterva_array_t *arr;
    CATERVA_ERROR(caterva_from_buffer(data, size, &params, &storage, &arr));

    uint8_t *cframe;
    int64_t cframe_len;
    bool needs_free;
    CATERVA_ERROR(caterva_to_cframe(ctx, arr, &cframe, &cframe_len, &needs_free));

    caterva_array_t *dest;
    CATERVA_ERROR(caterva_from_cframe(ctx, cframe, cframe_len, true, &dest));

    /* Fill dest array with caterva_array_t data */
    uint8_t *data_dest = malloc(size);
    CATERVA_ERROR(caterva_to_buffer(ctx, dest, data_dest, size));

    for (int i = 0; i < nelem; ++i) {
        if (data[i] != data_dest[i] && data[i] != i) {
            return -1;
        }
    }

    /* Free mallocs */
    free(data);
    free(data_dest);

    caterva_free(&arr);
    caterva_free(&dest);
    blosc2_free_ctx(ctx);

    return 0;
}
