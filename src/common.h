#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

#define MEM_TYPE 1
#define SSD_TYPE 2
#define HDD_TYPE 3
//* KEY and VALUES are of of char string type.
#define KEY_LEN 8
#define VAL_LEN 8

typedef struct{
    uint8_t _Alignas(16) key[KEY_LEN];
    uint8_t value[VAL_LEN];
} entry;

#endif // COMMON_H