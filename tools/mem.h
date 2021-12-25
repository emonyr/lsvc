#ifndef __MEM_TOOL_H__
#define __MEM_TOOL_H__

#define MEM_MAX_SIZE 65535

extern void * mem_alloc(unsigned int size);
extern void mem_free(void *p);


#endif	/* __MEM_TOOL_H__ */