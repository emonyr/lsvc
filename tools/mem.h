#ifndef __MEM_TOOL_H__
#define __MEM_TOOL_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MEM_MAX_SIZE 65535

extern void * mem_alloc(unsigned int size);
extern void mem_free(void *p);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif	/* __MEM_TOOL_H__ */