/*********************************************************************
  Blosc - Blocked Shuffling and Compression Library

  Copyright (C) 2021  The Blosc Developers <blosc@blosc.org>
  https://blosc.org
  License: BSD 3-Clause (see LICENSE.txt)

  See LICENSE.txt for details about copyright and rights to use.
**********************************************************************/


#include "test_common.h"


CUTEST_TEST_DATA(full) {
    blosc2_storage *b_storage;
};


CUTEST_TEST_SETUP(full) {
    blosc2_init();

    // Add parametrizations
    CUTEST_PARAMETRIZE(typesize, uint8_t, CUTEST_DATA(
            1, 2, 4, 8
    ));
    CUTEST_PARAMETRIZE(shapes, _test_shapes, CUTEST_DATA(
            {0, {0}, {0}, {0}}, // 0-dim
            {1, {5}, {3}, {2}}, // 1-idim
            {2, {20, 0}, {7, 0}, {3, 0}}, // 0-shape
            {2, {20, 10}, {7, 5}, {3, 5}}, // 0-shape
            {2, {14, 10}, {8, 5}, {2, 2}}, // general,
            {3, {12, 10, 14}, {3, 5, 9}, {3, 4, 4}}, // general
            {3, {10, 21, 20, 5}, {8, 7, 15, 3}, {5, 5, 10, 1}}, // general,
    ));
    CUTEST_PARAMETRIZE(backend, _test_backend, CUTEST_DATA(
            {false, false},
            {true, false},
            {true, true},
            {false, true},
    ));
    CUTEST_PARAMETRIZE(fill_value, int8_t, CUTEST_DATA(
            3, 113, 33, -5
    ));
}


CUTEST_TEST_TEST(full) {
    CUTEST_GET_PARAMETER(backend, _test_backend);
    CUTEST_GET_PARAMETER(shapes, _test_shapes);
    CUTEST_GET_PARAMETER(typesize, uint8_t);
    CUTEST_GET_PARAMETER(fill_value, int8_t);


    char *urlpath = "test_full.b2frame";
    blosc2_remove_urlpath(urlpath);

    caterva_params_t params;
    params.ndim = shapes.ndim;
    for (int i = 0; i < shapes.ndim; ++i) {
        params.shape[i] = shapes.shape[i];
    }

    blosc2_cparams cparams = BLOSC2_CPARAMS_DEFAULTS;
    blosc2_dparams dparams = BLOSC2_DPARAMS_DEFAULTS;
    cparams.nthreads = 2;
    cparams.compcode = BLOSC_BLOSCLZ;
    cparams.typesize = typesize;
    blosc2_storage b_storage = {.cparams=&cparams, .dparams=&dparams};
    data->b_storage = &b_storage;
    caterva_storage_t storage = {.b_storage=data->b_storage};
    if (backend.persistent) {
        storage.b_storage->urlpath = urlpath;
    }
    storage.b_storage->contiguous = backend.contiguous;
    for (int i = 0; i < shapes.ndim; ++i) {
        storage.chunkshape[i] = shapes.chunkshape[i];
        storage.blockshape[i] = shapes.blockshape[i];
    }
    int32_t blocknitems = 1;
    for (int i = 0; i < params.ndim; ++i) {
      blocknitems *= storage.blockshape[i];
    }
    storage.b_storage->cparams->blocksize = blocknitems * storage.b_storage->cparams->typesize;

    blosc2_context *ctx = blosc2_create_cctx(*storage.b_storage->cparams);

    /* Create original data */
    int64_t buffersize = typesize;
    for (int i = 0; i < shapes.ndim; ++i) {
        buffersize *= shapes.shape[i];
    }

    /* Create caterva_array_t with original data */
    caterva_array_t *src;
    uint8_t *value = malloc(typesize);
    switch (typesize) {
        case 8:
            ((int64_t *) value)[0] = (int64_t) fill_value;
            break;
        case 4:
            ((int32_t *) value)[0] = (int32_t) fill_value;
            break;
        case 2:
            ((int16_t *) value)[0] = (int16_t) fill_value;
            break;
        case 1:
            ((int8_t *) value)[0] = fill_value;
            break;
        default:
            break;
    }

    CATERVA_TEST_ASSERT(caterva_full(&params, &storage, value, &src));

    /* Fill dest array with caterva_array_t data */
    uint8_t *buffer_dest = malloc( buffersize);
    CATERVA_TEST_ASSERT(caterva_to_buffer(ctx, src, buffer_dest, buffersize));

    /* Testing */
    for (int i = 0; i < buffersize / typesize; ++i) {
        bool is_true = false;
        switch (typesize) {
            case 8:
                is_true = ((int64_t *) buffer_dest)[i] == fill_value;
                break;
            case 4:
                is_true = ((int32_t *) buffer_dest)[i] == fill_value;
                break;
            case 2:
                is_true = ((int16_t *) buffer_dest)[i] == fill_value;
                break;
            case 1:
                is_true = ((int8_t *) buffer_dest)[i] == fill_value;
                break;
            default:
                break;
        }
        CUTEST_ASSERT("Elements are not equals", is_true);
    }

    /* Free mallocs */
    free(buffer_dest);
    free(value);
    CATERVA_TEST_ASSERT(caterva_free(&src));
    blosc2_free_ctx(ctx);

    blosc2_remove_urlpath(urlpath);

    return CATERVA_SUCCEED;
}


CUTEST_TEST_TEARDOWN(full) {
    blosc2_destroy();
}

int main() {
    CUTEST_TEST_RUN(full);
}
