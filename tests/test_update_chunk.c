/*
  Copyright (C) 2020- The Blosc Development Team <blosc@blosc.org>
  http://blosc.org
  License: BSD (see LICENSE.txt)

  Creation date: 2020-09-23

  See LICENSE.txt for details about copyright and rights to use.
*/

#include <stdio.h>
#include "test_common.h"

#define CHUNKSIZE (200 * 1000)
#define NTHREADS (2)

/* Global vars */
int tests_run = 0;

typedef struct {
  int nchunks;
  int nupdates;
  char* urlpath;
  bool sequential;
} test_data;

test_data tdata;

typedef struct {
  int nchunks;
  int nupdates;
} test_ndata;

test_ndata tndata[] = {{10, 4},
                       {5,  0},
                       {33, 32},
                       {1,  0}};

typedef struct {
  bool sequential;
  char *urlpath;
}test_storage;

test_storage tstorage[] = {
    {false, NULL},  // memory - schunk
    {true, NULL},  // memory - frame
    {true, "test_update_chunk.b2frame"}, // disk - frame
    {false, "test_eframe_update_chunk.b2frame"}, // disk - eframe
};

static char* test_update_chunk(void) {
  static int32_t data[CHUNKSIZE];
  int32_t *data_dest = malloc(CHUNKSIZE * sizeof(int32_t));
  size_t isize = CHUNKSIZE * sizeof(int32_t);
  int dsize;
  blosc2_cparams cparams = BLOSC2_CPARAMS_DEFAULTS;
  blosc2_dparams dparams = BLOSC2_DPARAMS_DEFAULTS;
  blosc2_schunk* schunk;

  /* Initialize the Blosc compressor */
  blosc_init();

  /* Create a super-chunk container */
  cparams.typesize = sizeof(int32_t);
  cparams.compcode = BLOSC_BLOSCLZ;
  cparams.clevel = 5;
  cparams.nthreads = NTHREADS;
  dparams.nthreads = NTHREADS;
  blosc2_storage storage = {.cparams=&cparams, .dparams=&dparams,
                            .urlpath = tdata.urlpath,
                            .sequential = tdata.sequential};

  schunk = blosc2_schunk_new(storage);

  // Feed it with data
  for (int nchunk = 0; nchunk < tdata.nchunks; nchunk++) {
    for (int i = 0; i < CHUNKSIZE; i++) {
      data[i] = i + nchunk * CHUNKSIZE;
    }
    int nchunks_ = blosc2_schunk_append_buffer(schunk, data, isize);
    mu_assert("ERROR: bad append", nchunks_ > 0);
  }

  // Check that the chunks have been decompressed correctly
  for (int nchunk = 0; nchunk < tdata.nchunks; nchunk++) {
    dsize = blosc2_schunk_decompress_chunk(schunk, nchunk, (void *) data_dest, isize);
    mu_assert("ERROR: chunk cannot be decompressed correctly", dsize >= 0);
    for (int i = 0; i < CHUNKSIZE; i++) {
      mu_assert("ERROR: bad roundtrip", data_dest[i] == i + nchunk * CHUNKSIZE);
    }
  }

  for (int i = 0; i < tdata.nupdates; ++i) {
    // Create chunk
    for (int j = 0; j < CHUNKSIZE; ++j) {
      data[j] = i;
    }

    int32_t datasize = sizeof(int32_t) * CHUNKSIZE;
    int32_t chunksize = sizeof(int32_t) * CHUNKSIZE + BLOSC_MAX_OVERHEAD;
    uint8_t *chunk = malloc(chunksize);
    int csize = blosc2_compress_ctx(schunk->cctx, data, datasize, chunk, chunksize);
    mu_assert("ERROR: chunk cannot be compressed", csize >= 0);

    // Update a random position
    int pos = rand() % schunk->nchunks;
    int _nchunks = blosc2_schunk_update_chunk(schunk, pos, chunk, true);
    mu_assert("ERROR: chunk cannot be updated correctly", _nchunks > 0);
    free(chunk);

    // Assert updated chunk
    dsize = blosc2_schunk_decompress_chunk(schunk, pos, (void *) data_dest, isize);
    mu_assert("ERROR: chunk cannot be decompressed correctly", dsize >= 0);
    for (int j = 0; j < CHUNKSIZE; j++) {
      int32_t a = data_dest[j];
      mu_assert("ERROR: bad roundtrip", a == i);
    }
  }
  /* Free resources */
  if (!storage.sequential && storage.urlpath != NULL) {
    blosc2_remove_dir(storage.urlpath);
  }
  blosc2_schunk_free(schunk);
  /* Destroy the Blosc environment */
  blosc_destroy();

  free(data_dest);

  return EXIT_SUCCESS;
}

static char *all_tests(void) {
  for (int i = 0; i < sizeof(tstorage) / sizeof(test_storage); ++i) {
    for (int j = 0; j < sizeof(tndata) / sizeof(test_ndata); ++j) {
      tdata.sequential = tstorage[i].sequential;
      tdata.urlpath = tstorage[i].urlpath;
      tdata.nchunks = tndata[i].nchunks;
      tdata.nupdates = tndata[i].nupdates;

      mu_run_test(test_update_chunk);
    }
  }

  return EXIT_SUCCESS;
}


int main(void) {
  char *result;

  install_blosc_callback_test(); /* optionally install callback test */
  blosc_init();

  /* Run all the suite */
  result = all_tests();
  if (result != EXIT_SUCCESS) {
    printf(" (%s)\n", result);
  }
  else {
    printf(" ALL TESTS PASSED");
  }
  printf("\tTests run: %d\n", tests_run);

  blosc_destroy();

  return result != EXIT_SUCCESS;
}
