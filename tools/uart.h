#ifndef __UART_TOOL_H__
#define __UART_TOOL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void uart_set_speed(int fd, int speed);
int uart_set_parity(int fd, uint8_t databits, uint8_t stopbits, uint8_t parity);
int uart_init(const char *dev, int baud_rate, uint8_t databits,
                    uint8_t parity, uint8_t stopbits);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __UART_TOOL_H__ */
