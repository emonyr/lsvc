#ifndef __FILE_TOOL_H__
#define __FILE_TOOL_H__


extern int file_read_content(const char *filename,char *buffer,
									int size, long offset);
extern int file_write_content(const char *filename,char *buffer,
									int size, long offset);
extern int file_append_content(const char *filename,char *buffer,
									int size);
extern int file_get_size(const char *filename);
extern char *file_get_striped_name(const char *filename);



#endif	/* __FILE_TOOL_H__ */