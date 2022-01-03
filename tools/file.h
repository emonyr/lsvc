#ifndef __FILE_TOOL_H__
#define __FILE_TOOL_H__

#ifdef __cplusplus
extern "C" {
#endif

extern int file_read_content(const char *filename,char *buffer,
									int size, long offset);
extern int file_write_content(const char *filename,char *buffer,
									int size, long offset);
extern int file_append_content(const char *filename,char *buffer,
									int size);
extern int file_get_size(const char *filename);
extern char *file_get_striped_name(const char *filename);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif	/* __FILE_TOOL_H__ */