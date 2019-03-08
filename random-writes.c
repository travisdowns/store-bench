
// as a hack we just include pcg_basic.c (renamed to .c.h) so the rand functions can be inlined
#include "pcg_basic.c.h"

#include "common.h"

#include <inttypes.h>
#include <assert.h>

typedef uint64_t lcg_state;

#ifdef USE_PCG
#define RAND_FUNC pcg32_random_r
#define RAND_INIT PCG32_INITIALIZER
typedef pcg32_random_t rng_state;
#else
#define RAND_FUNC lcg_next
typedef lcg_state rng_state;
#define RAND_INIT 0x1234567890ABCDEFull
#endif


int random_pcg(size_t iters, char* a1, size_t size1, char *a2, size_t size2) {
    pcg32_random_t rng = PCG32_INITIALIZER;
    uint32_t total = 0;
    do {
        total += pcg32_random_r(&rng);
    } while (--iters > 0);
    return total;
}

uint32_t lcg_next(lcg_state* state) {
    uint64_t newstate = *state * 6364136223846793005ull + 1ull;
    *state = newstate;
    return newstate >> 32;
}

int random_lcg(size_t iters, char* a1, size_t size1, char *a2, size_t size2) {
    lcg_state rng = 123;
    uint32_t total = 0;
    do {
        total += lcg_next(&rng);
    } while (--iters > 0);
    return total;
}

int random_writes(size_t iters, char* a1, size_t size1, char *a2, size_t size2) {
    rng_state rng = RAND_INIT;
    do {
        uint32_t val = RAND_FUNC(&rng);
        a1[(val & (size1 - 1))      ] = 1;
        a2[(val & (size2 - 1)) ^ (1 << 16)] = 2;
    } while (--iters > 0);
    return 0;
}


