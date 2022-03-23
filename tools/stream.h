#ifndef __STREAM_TOOL_H__
#define __STREAM_TOOL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "list.h"
#include <stdint.h>

#define STREAM_DEFAULT_BLOCK_SIZE (4096)

typedef struct {
    uint8_t buffer[STREAM_DEFAULT_BLOCK_SIZE];
    void* end;
} __attribute__((packed)) stream_block_t;

typedef struct {
    stream_block_t** blocks;
    uint8_t* w;
    uint8_t* r;
    size_t current;
    size_t nblock;
    size_t bytes_written;
    size_t bytes_available;
} __attribute__((packed)) stream_t;


extern void* stream_create(void);
extern size_t stream_peek(stream_t* s);
extern int stream_push(stream_t* s, void* _payload, size_t len);
extern int stream_pop(stream_t* s, size_t needed, void* _bucket);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __STREAM_TOOL_H__ */