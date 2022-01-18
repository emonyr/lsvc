#ifndef __LOG_SVC_H_
#define __LOG_SVC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "parallel.h"
#include "lsvc.h"

#define LOG_PORT "19000"

typedef enum {
	LOG_EV_GET_STATE = LSVC_LOG_EVENT,
	LOG_EV_REPORT_STATE,
	LOG_EV_SET_LEVEL,
}log_svc_event_t;

typedef enum {
	LOG_LEVEL_FATAL=0,
	LOG_LEVEL_ERROR,
	LOG_LEVEL_WARNING,
	LOG_LEVEL_INFO,
	LOG_LEVEL_DEBUG,
}log_level_t;

typedef struct {
	log_level_t level;
	parallel_spin_t lock;
}__attribute__ ((packed))log_svc_state_t;

typedef struct {
	log_level_t level;
}log_ctl_t;

#ifndef log_fatal
#define log_fatal(format...) log_impl(stderr, LOG_LEVEL_FATAL, __FILE__, __LINE__, __func__, format)
#endif

#ifndef log_err
#define log_err(format...) log_impl(stderr, LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, format)
#endif

#ifndef log_warn
#define log_warn(format...) log_impl(stdout, LOG_LEVEL_WARNING, __FILE__, __LINE__, __func__, format)
#endif

#ifndef log_info
#define log_info(format...) log_impl(stdout, LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, format)
#endif

#ifndef log_debug
#define log_debug(format...) log_impl(stdout, LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, format)
#endif

#ifndef log_to_file
#define log_to_file(file,format...) \
	do{ \
		FILE *fp = fopen(file,"a+"); \
		log_impl(fp, LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, format); \
		fflush(fp); \
		fclose(fp); \
	}while(0)
#endif

extern int log_impl(FILE *output, int level, const char* filename, int line, 
						const char* function,const char *format, ...);
extern void log_hex_dump(unsigned char *data, int size);

extern lsvc_t log_svc;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif	/* __LOG_SVC_H_ */