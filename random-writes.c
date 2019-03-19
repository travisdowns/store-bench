
// as a hack we just include pcg_basic.c (renamed to .c.h) so the rand functions can be inlined
#include "pcg_basic.c.h"

#include "common.h"
#include "hedley.h"

#include <inttypes.h>
#include <assert.h>
#include <immintrin.h>

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


int write_random_singleu(size_t iters, char* a1, size_t size1, char *a2, size_t size2) {
    rng_state rng = RAND_INIT;
    iters /= 8u;
    do {
        uint64_t val = RAND_FUNC(&rng);
        a2[((val              ) & (size2 - 1))] = 2;
        a2[((val + 0xd6b45560u) & (size2 - 1))] = 3;
        a2[((val + 0x23a7b9cau) & (size2 - 1))] = 4;
        a2[((val + 0x60776172u) & (size2 - 1))] = 5;
        a2[((val + 0x43b006cau) & (size2 - 1))] = 6;
        a2[((val + 0xa8b0af69u) & (size2 - 1))] = 7;
        a2[((val + 0x66da7813u) & (size2 - 1))] = 8;
        a2[((val + 0xcc667058u) & (size2 - 1))] = 9;
    } while (--iters > 0);
    return 0;
}

int write_random_single(size_t iters, char* a1, size_t size1, char *a2, size_t size2) {
    rng_state rng = RAND_INIT;
    do {
        uint32_t val = RAND_FUNC(&rng);
        a2[(val & (size2 - 1))] = 2;
    } while (--iters > 0);
    return 0;
}

int write_linear(size_t iters, char* a1, size_t size1, char *a2, size_t size2) {
    size_t index = 0;
    do {
        a2[index & (size2 - 1)] = iters;
        index += 64;
    } while (--iters > 0);
    return 0;
}

int write_linear_sfence(size_t iters, char* a1, size_t size1, char *a2, size_t size2) {
    size_t index = 0;
    do {
        a2[index & (size2 - 1)] = iters;
        index += 64;
        _mm_sfence();
    } while (--iters > 0);
    return 0;
}

HEDLEY_ALWAYS_INLINE
static inline int write_linearT(size_t iters, char* a1, size_t size1, char *a2, size_t size2, void (*fence)()) {
    size_t index = 0, index2 = 0;
    do {
        a2[index & (size2 - 1)] = iters;
        size_t temp = 0;
        fence();
        a2[(index & (size2 - 1)) + 1] = iters;
        temp += a1[index2 + temp] & 0x100;
        temp += a1[index2 + temp] & 0x100;
        temp += a1[index2 + temp] & 0x100;
        temp += a1[index2 + temp] & 0x100;
        temp += a1[index2 + temp] & 0x100;
        //assert(temp == 0);
        index += 64 + temp;
        index2 = (index2 + 64) & (size1 - 1);
    } while (--iters > 0);
    return 0;
}

static inline void lfence() {
    _mm_lfence();
}

static inline void sfence() {
    _mm_sfence();
}

int write_linearHL(size_t iters, char* a1, size_t size1, char *a2, size_t size2) {
    return write_linearT(iters, a1, size1, a2, size2, lfence);
}

int write_linearHS(size_t iters, char* a1, size_t size1, char *a2, size_t size2) {
    return write_linearT(iters, a1, size1, a2, size2, sfence);
}

int read_linear(size_t iters, char* a1, size_t size1, char *a2, size_t size2) {
    size_t index = 0;
    char total = 0;
    do {
        total += a2[index & (size2 - 1)];
        index += 64;
    } while (--iters > 0);
    return total;
}

int writes_inter(size_t iters, char* a1, size_t size1, char *a2, size_t size2) {
    rng_state rng = RAND_INIT;
    do {
        uint32_t val = RAND_FUNC(&rng);
        a1[(val & (size1 - 1))] = 1;
        a2[(val & (size2 - 1))] = 2;
    } while (--iters > 0);
    return 0;
}

// unroll by 2 and reorder writes to the same region to make them adjacent
int writes_inter_u2(size_t iters, char* a1, size_t size1, char *a2, size_t size2) {
    rng_state rng = RAND_INIT;
    iters /= 2; // cut the iterations in half since we do double the work in each itr
    do {
        uint32_t val1 = RAND_FUNC(&rng);
        uint32_t val2 = RAND_FUNC(&rng);
        a1[(val1 & (size1 - 1))] = 1;
        a1[(val2 & (size1 - 1))] = 1;
        a2[(val1 & (size2 - 1))] = 2;
        a2[(val2 & (size2 - 1))] = 2;
    } while (--iters > 0);
    return 0;
}

// unroll by 4 and reorder writes to the same region to make them adjacent
int writes_inter_u4(size_t iters, char* a1, size_t size1, char *a2, size_t size2) {
    rng_state rng = RAND_INIT;
    iters /= 4;
    do {
        uint32_t val1 = RAND_FUNC(&rng);
        uint32_t val2 = RAND_FUNC(&rng);
        uint32_t val3 = RAND_FUNC(&rng);
        uint32_t val4 = RAND_FUNC(&rng);
        a1[(val1 & (size1 - 1))] = 1;
        a1[(val2 & (size1 - 1))] = 1;
        a1[(val3 & (size1 - 1))] = 1;
        a1[(val4 & (size1 - 1))] = 1;
        a2[(val1 & (size2 - 1))] = 2;
        a2[(val2 & (size2 - 1))] = 2;
        a2[(val3 & (size2 - 1))] = 2;
        a2[(val4 & (size2 - 1))] = 2;
    } while (--iters > 0);
    return 0;
}

int writes_inter_sfenceA(size_t iters, char* a1, size_t size1, char *a2, size_t size2) {
    rng_state rng = RAND_INIT;
    do {
        uint32_t val = RAND_FUNC(&rng);
        a1[(val & (size1 - 1))] = 1;
        _mm_sfence();
        a2[(val & (size2 - 1))] = 2;
    } while (--iters > 0);
    return 0;
}

int writes_inter_sfenceB(size_t iters, char* a1, size_t size1, char *a2, size_t size2) {
    rng_state rng = RAND_INIT;
    do {
        uint32_t val = RAND_FUNC(&rng);
        a1[(val & (size1 - 1))] = 1;
        a2[(val & (size2 - 1))] = 2;
        _mm_sfence();
    } while (--iters > 0);
    return 0;
}

int writes_inter_sfenceC(size_t iters, char* a1, size_t size1, char *a2, size_t size2) {
    rng_state rng = RAND_INIT;
    do {
        uint32_t val = RAND_FUNC(&rng);
        a1[(val & (size1 - 1))] = 1;
        _mm_sfence();
        a2[(val & (size2 - 1))] = 2;
        _mm_sfence();
    } while (--iters > 0);
    return 0;
}

int random_read2(size_t iters, char* a1, size_t size1, char *a2, size_t size2) {
    char total = 0;
    rng_state rng = RAND_INIT;
    do {
        uint32_t val = RAND_FUNC(&rng);
        total += a1[(val & (size1 - 1))];
        total += a2[(val & (size2 - 1))];
    } while (--iters > 0);
    return total;
}

