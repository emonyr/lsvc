#ifndef __UART_TOOL_H__
#define __UART_TOOL_H__

#ifdef __cplusplus
extern "C" {
#endif


void uart_set_speed(int fd, int speed);
int uart_set_parity(int fd, unsigned char databits, unsigned char stopbits, unsigned char parity);
int uart_init(const char *dev, int baud_rate, unsigned char databits,
                    unsigned char parity, unsigned char stopbits);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __UART_TOOL_H__ */
