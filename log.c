#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h> 
#include <sys/time.h>
#include <time.h>

#include "mem.h"
#include "lbus.h"
#include "file.h"
#include "log.h"

log_svc_state_t log_state = {
	.level = LOG_LEVEL_INFO,
	.mtx = PTHREAD_MUTEX_INITIALIZER,
};

int log_svc_state(void *_msg)
{
	log_svc_state_t *s = &log_state;
	
	//log_to_file("/tmp/log.log", "%d\n", s->v);

	return lsvc_event_send(LOG_EV_GET_STATE, s, sizeof(log_svc_state_t), 
							LMSG_RESPONSE, 1);
}


/* 
	'F' <--> LOG_LEVEL_FATAL LOG_LEVEL_EMERG
	'E' <--> LOG_LEVEL_ERROR
	'W' <--> LOG_LEVEL_WARNING
	'I' <--> LOG_LEVEL_INFO
	'D' <--> LOG_LEVEL_DEBUG	
*/

char *log_level_str[] = {
	"LOG_LEVEL_FATAL",
	"LOG_LEVEL_ERROR",
	"LOG_LEVEL_WARNING",
	"LOG_LEVEL_INFO",
	"LOG_LEVEL_DEBUG",
};

char log_level_ch[] = {'F', 'E', 'W', 'I', 'D'};

char *log_level_color[] = {	"\033[1;31m", 	//'F'
							"\033[1;31m", 	//'E'
							"\033[1;33m", 	//'W'
							"\033[1;32m", 	//'I'
							"\033[0;37m", 	//'D'
							};



int log_impl(FILE *output, int level, const char* filename, int line, 
						const char* function,const char *format, ...)
{
	struct timeval _tv;
	time_t cur_time = 0;
	struct tm _tm = {0};
	
	if(level > log_state.level)
		return 1;
	
	gettimeofday(&_tv, NULL);
	cur_time = _tv.tv_sec;
	if(!gmtime_r(&cur_time, &_tm))
		return -1;

	fprintf(output, "%s[%02d-%02d-%02d %02d:%02d:%02d.%03d][%s][%s:%d<%s>] \033[0m", 
						log_level_color[level],
						_tm.tm_year + 1900,
						_tm.tm_mon + 1,
						_tm.tm_mday, 
						(_tm.tm_hour + 8) % 24, 
						_tm.tm_min, _tm.tm_sec,
						(unsigned int)_tv.tv_usec/1000, 
						log_level_str[level],
						file_get_striped_name(filename), 
						line, function);
	
	va_list arg;
	va_start(arg, format);
	vfprintf(output, format, arg);
	va_end(arg);
	
	return 0;
}

void log_hex_dump(unsigned char *data, int size)
{
	char ascii[17];
	size_t i, j;
	ascii[16] = '\0';
	for (i = 0; i < size; ++i) {
		printf("%02X ", ((unsigned char*)data)[i]);
		if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
			ascii[i % 16] = ((unsigned char*)data)[i];
		} else {
			ascii[i % 16] = '.';
		}
		if ((i+1) % 8 == 0 || i+1 == size) {
			printf(" ");
			if ((i+1) % 16 == 0) {
				printf("|  %s \n", ascii);
			} else if (i+1 == size) {
				ascii[(i+1) % 16] = '\0';
				if ((i+1) % 16 <= 8) {
					printf(" ");
				}
				for (j = (i+1) % 16; j < 16; ++j) {
					printf("   ");
				}
				printf("|  %s \n", ascii);
			}
		}
	}

	return;
}

int log_set_level(log_level_t level)
{
	log_svc_state_t *s = &log_state;
	
	if(level < LOG_LEVEL_FATAL || level > LOG_LEVEL_DEBUG)
		return -1;
	
	s->level = level;
	log_info("log level changed to: %s\n", log_level_str[s->level]);
	
	return 0;
}


/*
 * lsvc api implementation
 */

int log_svc_init(void *runtime)
{
	log_set_level(LOG_LEVEL_DEBUG);
	return 0;
}

int log_svc_exit(void *runtime)
{
	
	return 0;
}

/*
 * shell command example
 * #set log level
 * /usr/local/bin/lsvc log -l 2
 */
int log_svc_getopt(int argc, const char *argv[], void **msg)
{
	int opt,err=0;
	lbus_msg_t *m;
	log_ctl_t *ctl;

	*msg = lbus_msg_new(sizeof(log_ctl_t));
	if (!*msg) {
		log_err("Failed to alloc msg buffer\n");
		return -1;
	}

	m = *msg;
	ctl = m->payload;
	while((opt = getopt(argc, (void *)argv, "l:")) != -1){
		switch (opt) {
			case 'l':
				m->event = LOG_EV_SET_LEVEL;
				ctl->level = atoi(optarg);
				break;

			case '?':
			case '-':
				err = -1;
				break;
		}
	}
	
	return err;
}

int log_svc_ioctl(void *runtime, void *_msg)
{
	int err = -1;
	lbus_msg_t *msg = _msg;
	log_ctl_t *ctl = msg->payload;
	log_debug("event 0x%08X\n", msg->event);
	
	switch(msg->event){
		case LOG_EV_GET_STATE:
			err = log_svc_state(msg);
		break;
		
		case LOG_EV_SET_LEVEL:
			err = log_set_level(ctl->level);
		break;
	}
	
	if(err < 0)
		log_err("ioctl failed: ev 0x%08X\n", msg->event);
	
	return err;
}

lsvc_t log_svc = {
	.ev_base = LSVC_LOG_EVENT,
	.name = "log",
	.getopt = log_svc_getopt,
	.init = log_svc_init,
	.exit = log_svc_exit,
	.ioctl = log_svc_ioctl,
};
