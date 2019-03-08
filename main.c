#define _GNU_SOURCE

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <inttypes.h>

#include <sys/mman.h>
#include <math.h>
#include <time.h>

#include "common.h"

#include "cycle-timer.h"
#include "huge-alloc.h"
#include "perf-timer.h"

#include <emmintrin.h>
#include <immintrin.h>

#include <sched.h>

bool verbose;
bool summary;

int getenv_int(const char *var, int def) {
    const char *val = getenv(var);
    return val ? atoi(val) : def;
}

bool getenv_bool(const char *var) {
    const char *val = getenv(var);
    return val && strcmp(val, "1") == 0;
}


typedef struct {
    const char *name;
    store_function *f;
    const char *desc;
    // expected number of L1 and L2 hits (L2 hits implying L1 misses)
    int l1_hits;
    int l2_hits;
    // a multiplier applied to the number of "kernel loops" (accesses the test is
    // request to perform), useful for tests that have sigifnicant startup overhead
    int loop_mul;
} test_description;




//                                                                                      /---------- l1_hits
//                                                                                      |  /------- l2_hits
//                                                                                      |  |   /--- loop_mul
const test_description all_funcs[] = {   //                                             v  v   v
    { "c++"      , random_writes     , "c++ version of fixed L1 + 16-stride L2 writes", 1, 1,  1 },
    { "lcg"      , random_lcg        , "c++ version of fixed L1 + 16-stride L2 writes", 1, 1,  1 },
    { "pcg"      , random_pcg        , "c++ version of fixed L1 + 16-stride L2 writes", 1, 1,  1 },
    {}  // sentinel
};


void flushmem(void *p, size_t size) {
    for (size_t i = 0; i < size; i += 64) {
        _mm_clflush((char *)p + i);
    }
    _mm_mfence();
}

char *alloc(size_t size) {
    // static volatile int zero;
    size_t grossed_up = size * 2 + 1000;
    char *p = huge_alloc(grossed_up, !summary);
    memset(p, 0x13, grossed_up);

    flushmem(p, grossed_up);

    return p;
}

void pinToCpu(int cpu) {
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpu, &set);
    if (sched_setaffinity(0, sizeof(set), &set)) {
        assert("pinning failed" && false);
    }
}


void usageError() {
    fprintf(stderr,
            "Usage:\n"
            "\tbench TEST_NAME\n"
            "\n TEST_NAME is one of:\n\n"
            );

    for (const test_description* desc = all_funcs; desc->name; desc++) {
        printf(" %s\n\t%s\n", desc->name, desc->desc);
    }
    exit(EXIT_FAILURE);
}

/* dump the tests in a single space-separate strings, perhaps convenient so you can do something like:
       for test in $(W_DUMPTESTS=1 ./weirdo-main); do ./weirdo-main $test; done
    to run all tests. */
void dump_tests() {
    for (const test_description* desc = all_funcs; desc->name; desc++) {
        printf("%s ", desc->name);
    }
}

size_t parse_kib(const char* strval) {
    int val = atoi(strval);
    if (val <= 0) {
        fprintf(stderr, "bad start/stop value: '%s'\n", strval);
        usageError();
    }
    size_t ret = 1;
    while (ret < (size_t)val) {
        ret *= 2;
    }
    return ret;
}

/* return the alignment of the given pointer
   i.e., the largest power of two that divides it */
size_t get_alignment(void *p) {
    return (size_t)((1UL << __builtin_ctzl((uintptr_t)p)));
}

void update_min_counts(bool first, event_counts* min, event_counts cur) {
    for (size_t i = 0; i < MAX_EVENTS; i++) {
        min->counts[i] = (first || cur.counts[i] < min->counts[i] ? cur.counts[i] : min->counts[i]);
    }
}

void print_count_deltas(event_counts delta, uint64_t divisor, const char *format, const char *delim) {
    for (size_t i=0; i < num_counters(); i++) {
        if (i) printf("%s", delim);
        printf(format, (double)delta.counts[i] / divisor);
    }
}

int main(int argc, char** argv) {

    summary              = getenv_bool("SUMMARY");
    verbose              = !summary; // && getenv_bool("W_VERBOSE");
    bool dump_tests_flag = getenv_bool("DUMPTESTS");
    bool use_counters    = getenv_bool("CPU_COUNTERS");
    bool allow_alias     = getenv_bool("ALLOW_ALIAS");
    bool plot            = getenv_bool("PLOT"); // output the data in csv format suitable for plotting

    int array1_kib = getenv_int("ARRAY1_SIZE", 128);
    int pincpu     = getenv_int("PINCPU",    1);

    if (dump_tests_flag) {
        dump_tests();
        exit(EXIT_SUCCESS);
    }

    if (argc != 2 && argc != 4) {
        fprintf(stderr, "Must provide 2 or 4 args\n\n");
        usageError();
    }

    size_t start_kib = (argc == 4 ? parse_kib(argv[2]) :   4);
    size_t  stop_kib = (argc == 4 ? parse_kib(argv[3]) : 512);

    const char* fname = argc >= 2 ? argv[1] : "asm";
    const test_description *test = 0;
    for (const test_description* desc = all_funcs; desc->name; desc++) {
        if (strcmp(desc->name, fname) == 0) {
            test = desc;
            break;
        }
    }

    if (!test) {
        fprintf(stderr, "Bad test name: %s\n", fname);
        usageError();
    }

    pinToCpu(pincpu);

    cl_init(!summary);

    // run the whole test repeat_count times, each of which calls the test function iters times, 
    // and each test function should loop kernel_loops times (with 1 or 2 stores), or equivalent with unrolling
    unsigned repeat_count = 10;
    size_t iters = 1000;
    size_t output1_size  = array1_kib * 1024;  // in bytes
    size_t kernel_loops  = output1_size * test->loop_mul / 10;

    char *output1 = alloc(output1_size * 2);
    // adjust the second array by a page to avoid aliasing (not 4K aliasing though), unless allow_alias is set 
    char *output2 = alloc(stop_kib * 1024 * 2) + (allow_alias ? 0 : 4096);

    if (!summary) {
        fprintf(stderr, "Running test %s : %s\n", test->name, test->desc);
    }

    if (verbose)  {
        fprintf(stderr, "pinned to cpu  : %10d\n", pincpu);
        fprintf(stderr, "current cpu    : %10d\n", sched_getcpu());
        fprintf(stderr, "array1 size    : %10zu KiB\n", output1_size / 1024);
        fprintf(stderr, "array1 align   : %10zu\n", get_alignment(output1));
        fprintf(stderr, "array2 start   : %10zu KiB\n", start_kib);
        fprintf(stderr, "array2 stop    : %10zu KiB\n",  stop_kib);
        fprintf(stderr, "array2 align   : %10zu\n", get_alignment(output2));
        fprintf(stderr, "expected number of L1 store hits: %zu\n", (size_t)iters * kernel_loops * repeat_count * test->l1_hits);
        fprintf(stderr, "expected number of L2 store hits: %zu\n", (size_t)iters * kernel_loops * repeat_count * test->l2_hits);
    }

    if (use_counters) {
        setup_counters(!plot);
    }

    if (!summary) {
        fprintf(stderr, "Starting main loop after %zu ms\n", (size_t)clock() * 1000u / CLOCKS_PER_SEC);
    }

    double total_iters = (double)kernel_loops * iters;

#define DELIM "%1$s"
    const char *header_delim = plot ? "," : " | ";
    if (plot) {
        printf("array2 KiB" DELIM "cycles/iter" DELIM, header_delim);
    } else {
        printf("array2 KiB" DELIM "iter" DELIM "cyc/iter" DELIM, header_delim);
    }
    print_counter_headings(header_delim);
    printf("\n");
    for (size_t array2_kib = start_kib; array2_kib <= stop_kib; array2_kib *= 2) {
        double min_cycles = UINT64_MAX, max_cycles = 0;
        event_counts min_counts = {};
        for (unsigned repeat = 0; repeat < repeat_count; repeat++) {
            event_counts counts_before = read_counters(); 
            cl_timepoint start = cl_now();
            for (int c = iters; c-- > 0;) {
                test->f(kernel_loops, output1, output1_size, output2, array2_kib * 1024);
                _mm_lfence(); // prevent inter-iteration overlap
                _mm256_zeroupper();
            }
            cl_timepoint end = cl_now();
            event_counts counts_after = read_counters(); 
            cl_interval delta_nanos = cl_delta(start, end);
            double cycles = cl_to_cycles(delta_nanos);
            event_counts counts_delta = calc_delta(counts_before, counts_after);
            if (!plot) {
                printf("%10zu %6d %10.2f  ", array2_kib, repeat, cycles / total_iters);
                print_count_deltas(counts_delta, total_iters, " %12.2f", "  ");
                printf("\n");
            }
            min_cycles = fmin(min_cycles, cycles);
            max_cycles = fmax(max_cycles, cycles);
            update_min_counts(repeat == 0, &min_counts, counts_delta);
        }
        if (plot) {
            printf("%zu,%.4f,", array2_kib, min_cycles / total_iters);
            print_count_deltas(min_counts, total_iters, "%.4f", ",");
            printf("\n");
        } else {
            const char *fmt = "%10zu %6s %10.2f  ";
            printf(fmt, array2_kib, "min", min_cycles / total_iters);
            print_count_deltas(min_counts, total_iters, " %12.2f", "  ");
            printf("\n");
            printf(fmt, array2_kib, "max", max_cycles / total_iters);
            printf("\n----------------------------------------\n");
        }
    }


}


