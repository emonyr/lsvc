#include <stdio.h>
#include <stdlib.h>

#include "file.h"

int file_read_content(const char *filename,char *buffer,
									size_t size, long offset)
{
	int nbytes;
	FILE *fp = NULL;
	fp = fopen(filename, "r");
	if(!fp) return -1;

	fseek(fp,offset,SEEK_SET);
    nbytes = fread(buffer, 1, size, fp);
	fflush(fp);
    fclose(fp);
	
	return nbytes;
}


int file_write_content(const char *filename,char *buffer,
									size_t size, long offset)
{
	int nbytes;
	FILE *fp = NULL;
	fp = fopen(filename, "w");
	if(!fp) return -1;
	
	fseek(fp,offset,SEEK_SET);
    nbytes = fwrite(buffer, 1, size, fp);
	fflush(fp);
    fclose(fp);
	
	return nbytes;
}


int file_append_content(const char *filename,char *buffer,
									size_t size)
{
	int nbytes;
	FILE *fp = NULL;
	fp = fopen(filename, "a+");
	if(!fp) return -1;
	
    nbytes = fwrite(buffer, 1, size, fp);
	fflush(fp);
    fclose(fp);
	
	return nbytes;
}


int file_get_size(const char *filename)
{
	int nbytes;
	FILE *fp = NULL;
	fp = fopen(filename, "r");
	if(!fp) return -1;
	
	fseek(fp,0,SEEK_END);
    nbytes = ftell(fp);
    fclose(fp);
	
	return nbytes;
}

char *file_get_striped_name(const char *filename)
{
	char *strip_filename = (char *)filename;

	for(int i=0; filename[i] != '\0'; i++){
		if(filename[i] == '/')
			strip_filename = (char *)&filename[i] + 1;
	}

	return strip_filename;
}
