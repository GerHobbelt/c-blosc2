/*********************************************************************
  Blosc - Blocked Shuffling and Compression Library

  Copyright (C) 2021  The Blosc Developers <blosc@blosc.org>
  https://blosc.org
  License: BSD 3-Clause (see LICENSE.txt)

  See LICENSE.txt for details about copyright and rights to use.
**********************************************************************/

#include "test_common.h"
#ifdef __GNUC__
#include <unistd.h>
#define FILE_EXISTS(urlpath) access(urlpath, F_OK)
#else
#include <io.h>
#define FILE_EXISTS(urlpath) _access(urlpath, 0)
#endif


typedef struct {
    int8_t ndim;
    int64_t shape[CATERVA_MAX_DIM];
    int32_t chunkshape[CATERVA_MAX_DIM];
    int32_t blockshape[CATERVA_MAX_DIM];
} test_shapes_t;


CUTEST_TEST_DATA(persistency) {
    caterva_ctx_t *ctx;
};


CUTEST_TEST_SETUP(persistency) {
    blosc2_init();
    caterva_config_t cfg = CATERVA_CONFIG_DEFAULTS;
    cfg.nthreads = 2;
    cfg.compcode = BLOSC_BLOSCLZ;
    caterva_ctx_new(&cfg, &data->ctx);

    // Add parametrizations
    CUTEST_PARAMETRIZE(itemsize, uint8_t, CUTEST_DATA(1, 2, 4, 8));
    CUTEST_PARAMETRIZE(shapes, test_shapes_t, CUTEST_DATA(
             {0, {0}, {0}, {0}}, // 0-dim
             {1, {10}, {7}, {2}}, // 1-idim
             {2, {100, 100}, {20, 20}, {10, 10}},
             {3, {100, 55, 23}, {31, 5, 22}, {4, 4, 4}},
             {3, {100, 0, 12}, {31, 0, 12}, {10, 0, 12}},
             {4, {50, 30, 31, 12}, {25, 20, 20, 10}, {5, 5, 5, 10}},
             {5, {1, 1, 1024, 1, 1}, {1, 1, 500, 1, 1}, {1, 1, 200, 1, 1}},
             {6, {5, 1, 100, 3, 1, 2}, {5, 1, 50, 2, 1, 2}, {2, 1, 20, 2, 1, 2}}
    ));
    CUTEST_PARAMETRIZE(backend, _test_backend, CUTEST_DATA(
            {true, true},
            {false, true},
    ));
}

CUTEST_TEST_TEST(persistency) {
    CUTEST_GET_PARAMETER(backend, _test_backend);
    CUTEST_GET_PARAMETER(shapes, test_shapes_t);
    CUTEST_GET_PARAMETER(itemsize, uint8_t);

    char* urlpath = "test_persistency.b2frame";

    caterva_remove(data->ctx, urlpath);

    caterva_params_t params;
    params.itemsize = itemsize;
    params.ndim = shapes.ndim;
    for (int i = 0; i < params.ndim; ++i) {
        params.shape[i] = shapes.shape[i];
    }

    caterva_storage_t storage = {0};
    if (backend.persistent) {
        storage.urlpath = urlpath;
    }
    storage.contiguous = backend.contiguous;
    for (int i = 0; i < params.ndim; ++i) {
        storage.chunkshape[i] = shapes.chunkshape[i];
        storage.blockshape[i] = shapes.blockshape[i];
    }


    /* Create original data */
    int64_t buffersize = itemsize;
    for (int i = 0; i < params.ndim; ++i) {
        buffersize *= shapes.shape[i];
    }
    uint8_t *buffer = malloc(buffersize);
    CUTEST_ASSERT("Buffer filled incorrectly", fill_buf(buffer, itemsize, buffersize / itemsize));

    /* Create caterva_array_t with original data */
    caterva_array_t *src;
    CATERVA_TEST_ASSERT(caterva_from_buffer(data->ctx, buffer, buffersize, &params, &storage,
                                            &src));

    caterva_array_t *dest;
    CATERVA_TEST_ASSERT(caterva_open(data->ctx, urlpath, &dest));

    /* Fill dest array with caterva_array_t data */
    uint8_t *buffer_dest = malloc(buffersize);
    CATERVA_TEST_ASSERT(caterva_to_buffer(data->ctx, dest, buffer_dest, buffersize));

    /* Testing */
    if (dest->nitems != 0) {
        for (int i = 0; i < buffersize / itemsize; ++i) {
            // printf("%d - %d\n", buffer[i], buffer_dest[i]);
            CUTEST_ASSERT("Elements are not equals!", buffer[i] == buffer_dest[i]);
        }
    }

    /* Free mallocs */
    free(buffer);
    free(buffer_dest);
    CATERVA_TEST_ASSERT(caterva_free(data->ctx, &src));
    CATERVA_TEST_ASSERT(caterva_free(data->ctx, &dest));

    caterva_remove(data->ctx, urlpath);

    return 0;
}


CUTEST_TEST_TEARDOWN(persistency) {
    caterva_ctx_free(&data->ctx);
    blosc2_destroy();
}

int main() {
    CUTEST_TEST_RUN(persistency);
}
