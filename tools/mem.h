#ifndef __MEM_TOOL_H__
#define __MEM_TOOL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#ifndef container_of
#define container_of(ptr, type, member) ({ \
        const __typeof__(((type *)0)->member ) *__mptr = (ptr); \
        (type *)((char *)__mptr - offsetof(type,member) );})
#endif 

extern void * mem_alloc(uint32_t size);
extern void mem_free(void *p);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif	/* __MEM_TOOL_H__ */