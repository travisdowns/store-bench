#ifndef COMMON_H_
#define COMMON_H_

#include <stdlib.h>

typedef int (store_function)(size_t iters, char* a1, size_t size1, char *a2, size_t size2);

store_function write_random_single;
store_function write_random_singleu;
store_function write_linear;
store_function write_linearHL;
store_function write_linearHS;
store_function write_linear_sfence;
store_function writes_inter;
store_function writes_inter_u2;
store_function writes_inter_u4;
store_function writes_inter_sfenceA;
store_function writes_inter_sfenceB;
store_function writes_inter_sfenceC;
store_function read_linear;
store_function random_read2;
store_function random_lcg;
store_function random_pcg;

#endif
