#ifndef __UART_TOOL_H__
#define __UART_TOOL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

void UART_setSpeed(int32_t fd, int32_t speed);
int32_t UART_setParity(int32_t fd,uint8_t databits,uint8_t stopbits,uint8_t parity);
int32_t UART_init(const char *dev,int32_t baud_rate,uint8_t databits,uint8_t parity,uint8_t stopbits);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __UART_TOOL_H__ */
