#include <stdio.h>

#include "lsvc.h"

extern int lsvc_handle_sys_command(int argc, const char *argv[]);
extern void lsvc_service_init(void);
extern void lsvc_service_exit(void);
extern void lsvc_main_loop(void);

int main(int argc, const char *argv[])
{	
	if (argc > 1) {
		lsvc_handle_sys_command(argc, argv);
		lsvc_main_loop();
		return 0;
	}
	
	lsvc_service_init();
	lsvc_main_loop();
	lsvc_service_exit();
	
    return 0;
}
