#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mem.h"
#include "stream.h"

#ifdef __cplusplus
extern "C" {
#endif

void* stream_create(void)
{
    stream_t* s = NULL;

    s = mem_alloc(sizeof(stream_t));
    if (!s) {
        fprintf(stderr, "%s failed at %d\n", __func__, __LINE__);
        goto out;
    }

    s->blocks = NULL;
    s->w = NULL;
    s->r = NULL;
    s->current = 0;
    s->nblock = 0;
    s->bytes_written = 0;
    s->bytes_available = 0;

out:
    return s;
}

static inline void* stream_extend(stream_t* s, size_t target_size)
{
    int new_nblock = 0, gap = 0;

    // add 2 more blocks for spare
    gap = (target_size / STREAM_DEFAULT_BLOCK_SIZE) + 2;
    new_nblock = s->nblock + gap;

    stream_block_t** new_blocks = mem_alloc(new_nblock * sizeof(stream_block_t*));
    if (!new_blocks) {
        fprintf(stderr, "%s failed at %d\n", __func__, __LINE__);
        goto FAIL;
    }

    memset(new_blocks, 0, new_nblock * sizeof(stream_block_t*));
    for (int i = s->nblock; i < new_nblock; i++) {
        new_blocks[i] = mem_alloc(sizeof(stream_block_t));
        if (!new_blocks[i]) {
            fprintf(stderr, "%s failed at %d\n", __func__, __LINE__);
            goto FAIL;
        }
        new_blocks[i]->end = new_blocks[i]->buffer + STREAM_DEFAULT_BLOCK_SIZE;
    }

    if (s->blocks) {
        memcpy(new_blocks, s->blocks, s->nblock * sizeof(stream_block_t*));
        mem_free(s->blocks);
    }

    s->blocks = new_blocks;
    s->nblock = new_nblock;
    s->w = (s->w ? s->w : s->blocks[0]->buffer);
    s->r = (s->r ? s->r : s->blocks[0]->buffer);
    s->bytes_available += (gap * STREAM_DEFAULT_BLOCK_SIZE);

    return s->w;

FAIL:
    if (new_blocks) {
        for (int i = s->nblock; i < new_nblock; i++) {
            if (new_blocks[i])
                mem_free(new_blocks[i]);
        }
        mem_free(new_blocks);
    }
    return NULL;
}

// void* stream_shrink(stream_t* s, size_t target_size)
// {
// }

static inline void* stream_resize(stream_t* s, size_t target_size)
{
    void* p = NULL;

    if (s->bytes_available < target_size) {
        p = stream_extend(s, target_size);
    } else {
        p = s->w;
    }

    return p;
}

size_t stream_peek(stream_t* s)
{
    return s->bytes_written;
}

static inline void stream_rotate(stream_t* s)
{
    if (s->current > 0) {
        void* temp = s->blocks[0];
        memcpy(&s->blocks[0], &s->blocks[1], (s->nblock - 1) * sizeof(stream_block_t*));
        s->blocks[s->nblock - 1] = temp;
        s->current--;
        s->bytes_available += STREAM_DEFAULT_BLOCK_SIZE;
    }

	s->r = s->blocks[0]->buffer;
}

int stream_push(stream_t* s, void* _payload, size_t len)
{
    uint8_t* payload = _payload;

    if (!stream_resize(s, len)) {
        fprintf(stderr, "%s failed at %d\n", __func__, __LINE__);
        return -1;
    }

    for (int i = 0; i < len; i++) {
        if (s->w == s->blocks[s->current]->end) {
            s->current++;
            s->w = s->blocks[s->current]->buffer;
        }

        *(s->w) = payload[i];
        s->w++;
        s->bytes_written++;
        s->bytes_available--;
    }

    return len;
}

int stream_pop(stream_t* s, size_t needed, void* _bucket)
{
    size_t pop_size = 0;
    uint8_t* bucket = _bucket;

    if (!s->r || s->bytes_written < needed || !bucket)
        return -1;

    pop_size = needed ? needed : s->bytes_written;

    for (int i = 0; i < pop_size; i++) {
        if (s->r == s->blocks[0]->end) {
            stream_rotate(s);
        }
        bucket[i] = *(s->r);
        s->r++;
        s->bytes_written--;
    }

    return pop_size;
}

#ifdef __cplusplus
} /* extern "C" */
#endif