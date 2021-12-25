#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mem.h"

//TODO: manage memory pool

void * mem_alloc(unsigned int size)
{
	void *p = NULL;
	
	if(!size || size > MEM_MAX_SIZE)
		return NULL;
	
	p = malloc(size);
	if(p){
		memset(p,0,size);
	}else{
		fprintf(stderr, "mem_alloc failed\n");
	}
	
	return p;
}

void mem_free(void *p)
{
	if(p)
		free(p);
}