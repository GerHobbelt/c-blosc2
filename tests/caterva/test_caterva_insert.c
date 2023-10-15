/*********************************************************************
  Blosc - Blocked Shuffling and Compression Library

  Copyright (C) 2021  The Blosc Developers <blosc@blosc.org>
  https://blosc.org
  License: BSD 3-Clause (see LICENSE.txt)

  See LICENSE.txt for details about copyright and rights to use.
**********************************************************************/

#include "test_common.h"

typedef struct {
    int8_t ndim;
    int64_t shape[CATERVA_MAX_DIM];
    int32_t chunkshape[CATERVA_MAX_DIM];
    int32_t blockshape[CATERVA_MAX_DIM];
    int64_t buffershape[CATERVA_MAX_DIM];
    int8_t axis;
    int64_t start;
} test_shapes_t;


CUTEST_TEST_SETUP(insert) {
  blosc2_init();

  // Add parametrizations
  CUTEST_PARAMETRIZE(typesize, uint8_t, CUTEST_DATA(
          1,
          2,
          4,
          8,
  ));

  CUTEST_PARAMETRIZE(backend, _test_backend, CUTEST_DATA(
          {false, false},
          {true, false},
          {true, true},
          {false, true},
  ));


  CUTEST_PARAMETRIZE(shapes, test_shapes_t, CUTEST_DATA(
          {1, {5}, {3}, {2}, {10}, 0, 5}, // insert at the end (old padding modified)
          {2, {18, 6}, {6, 6}, {3, 3}, {18, 12}, 1, 0}, // insert at the beginning
          {3, {12, 10, 14}, {3, 5, 9}, {3, 4, 4}, {12, 10, 18}, 2, 9}, // insert in the middle
          {4, {10, 10, 5, 5}, {5, 7, 3, 3}, {2, 2, 1, 1}, {10, 10, 5, 30}, 3, 3}, // insert in the middle

  ));
}

CUTEST_TEST_TEST(insert) {
  CUTEST_GET_PARAMETER(backend, _test_backend);
  CUTEST_GET_PARAMETER(shapes, test_shapes_t);
  CUTEST_GET_PARAMETER(typesize, uint8_t);

  char *urlpath = "test_insert_shape.b2frame";
  blosc2_remove_urlpath(urlpath);

  blosc2_cparams cparams = BLOSC2_CPARAMS_DEFAULTS;
  cparams.typesize = typesize;
  cparams.nthreads = 2;
  blosc2_storage b2_storage = {.cparams=&cparams};
  if (backend.persistent) {
    b2_storage.urlpath = urlpath;
  }
  b2_storage.contiguous = backend.contiguous;

  caterva_context_t *ctx = caterva_create_ctx(&b2_storage, shapes.ndim, shapes.shape,
                                              shapes.chunkshape, shapes.blockshape, NULL, 0);

  int64_t buffersize = typesize;
  for (int i = 0; i < ctx->ndim; ++i) {
    buffersize *= shapes.buffershape[i];
  }

  /* Create caterva_array_t filled with 1 */
  caterva_array_t *src;
  uint8_t *value = malloc(typesize);
  int8_t fill_value = 1;
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
  BLOSC_ERROR(caterva_full(ctx, &src, value));

  uint8_t *buffer = malloc(buffersize);
  fill_buf(buffer, typesize, buffersize / typesize);
  BLOSC_ERROR(caterva_insert(src, buffer, buffersize, shapes.axis, shapes.start));

  int64_t start[CATERVA_MAX_DIM] = {0};
  start[shapes.axis] = shapes.start;
  int64_t stop[CATERVA_MAX_DIM];
  for (int i = 0; i < shapes.ndim; ++i) {
    stop[i] = shapes.shape[i];
  }
  stop[shapes.axis] = shapes.start + shapes.buffershape[shapes.axis];

  /* Fill buffer with a slice from the new chunks */
  uint8_t *res_buffer = malloc(buffersize);
  BLOSC_ERROR(caterva_get_slice_buffer(src, start, stop, res_buffer,
                                         shapes.buffershape, buffersize));

  for (uint64_t i = 0; i < (uint64_t) buffersize / typesize; ++i) {
    switch (typesize) {
      case 8:
        CUTEST_ASSERT("Elements are not equal!",
                      ((uint64_t *) buffer)[i] == ((uint64_t *) res_buffer)[i]);
        break;
      case 4:
        CUTEST_ASSERT("Elements are not equal!",
                      ((uint32_t *) buffer)[i] == ((uint32_t *) res_buffer)[i]);
        break;
      case 2:
        CUTEST_ASSERT("Elements are not equal!",
                      ((uint16_t *) buffer)[i] == ((uint16_t *) res_buffer)[i]);
        break;
      case 1:
        CUTEST_ASSERT("Elements are not equal!",
                      ((uint8_t *) buffer)[i] == ((uint8_t *) res_buffer)[i]);
        break;
      default:
        CATERVA_TEST_ASSERT(BLOSC2_ERROR_INVALID_PARAM);
    }
  }
  /* Free mallocs */
  free(value);
  free(buffer);
  free(res_buffer);

  CATERVA_TEST_ASSERT(caterva_free(src));
  CATERVA_TEST_ASSERT(caterva_free_ctx(ctx));
  blosc2_remove_urlpath(urlpath);

  return 0;
}

CUTEST_TEST_TEARDOWN(insert) {
  blosc2_destroy();
}

int main() {
  CUTEST_TEST_RUN(insert);
}
