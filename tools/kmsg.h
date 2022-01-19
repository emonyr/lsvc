#ifndef _KMSG_H_
#define _KMSG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "parallel.h"

#define MAX_MSG_NUMBER  128

struct kmsg {
    uint32_t size;   // buffer capcacity
    uint32_t out;
    uint32_t in;
    uint8_t *data;
    int msg_number;
    int msg_part_len[MAX_MSG_NUMBER];
    parallel_spin_t lock;  
};


int kmsg_new(struct kmsg *buf, size_t size);

int kmsg_delete(struct kmsg *buf);

int kmsg_push(struct kmsg *buf, const char *data, size_t len);

int kmsg_check(struct kmsg *buf);

int kmsg_pop(struct kmsg *buf, char *data);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

