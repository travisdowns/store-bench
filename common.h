#ifndef COMMON_H_
#define COMMON_H_

#include <stdlib.h>

typedef int (store_function)(size_t iters, char* a1, size_t size1, char *a2, size_t size2);

store_function random_writes;
store_function random_lcg;
store_function random_pcg;

#endif
