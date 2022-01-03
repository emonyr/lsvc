#ifndef __MEM_TOOL_H__
#define __MEM_TOOL_H__

#ifdef __cplusplus
extern "C" {
#endif


extern void * mem_alloc(unsigned int size);
extern void mem_free(void *p);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif	/* __MEM_TOOL_H__ */