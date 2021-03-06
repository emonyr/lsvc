#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mem.h"

//TODO: manage memory pool

void * mem_alloc(uint32_t size)
{
	void *p = NULL;
	
	if(!size)
		return NULL;
	
	p = malloc(size);
	if(p){
		memset(p,0,size);
	}else{
		fprintf(stderr, "%s failed at %d\n", __func__, __LINE__);
	}
	
	return p;
}

void mem_free(void *p)
{
	if(p)
		free(p);
}