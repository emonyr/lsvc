#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
 
#include "uart.h"
 
 
int speed_arr[] = {B230400, B115200, B57600, B38400, B19200, B9600, B4800, B2400, B1800, B1200, B600, B300,};

int name_arr[] = {230400, 115200, 57600, 38400, 19200, 9600, 4800, 2400, 1800, 1200, 600, 300,};

/*uart_setSpeed()
 *@brief  set uart speed
 *@param  fd: uart file descriptor
 *@param  speed: baud rate to be set
 *@return	void
 */
void uart_set_speed(int fd, int speed)
{
	 int i;
	 struct termios Opt;
	 tcgetattr(fd, &Opt);

	 for(i=0;i<sizeof(speed_arr)/sizeof(int);i++){
		 if(speed == name_arr[i]){
			 tcflush(fd,TCIOFLUSH);
			 
			 //set baud rate
			 cfsetispeed(&Opt, speed_arr[i]);
			 cfsetospeed(&Opt, speed_arr[i]);
			 if(tcsetattr(fd, TCSANOW, &Opt) != 0)
				fprintf(stderr, "uart_SetSpeed: failed to set IO speed.\n");
		 }
	 }
	 tcflush(fd,TCIOFLUSH);
}


/*uart_setParity()
 *@brief  set uart parity
 *@param  fd: uart file descriptor
 *@param  databits: data bit (7,8)
 *@param  stopbits: stop bit (1,2)
 *@param  parity: parity bit (N,E,O,S)
 */
int uart_set_parity(int fd, uint8_t databits, uint8_t stopbits, uint8_t parity)
{
	 struct termios options;

	 if(tcgetattr(fd,&options) != 0)
		 fprintf(stderr, "uart_setParity: tcgetattr failed\n");
	 
	 //disable all character processing
	 //options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON | IXOFF);
	 //options.c_oflag &= ~OPOST;
	 //options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	 //options.c_cflag &= ~CSIZE;
	 cfmakeraw(&options);
	 
	  //set data bit
	 switch(databits){
		 case 7:
			 options.c_cflag |= CS7;
			 break;
		 
		 case 8:
			 options.c_cflag |= CS8;
			 break;
		 
		 default:
			 options.c_cflag |= CS8;
			 fprintf(stderr,"Unsupported data size,dafaulting to CS8\n");
			 return -1;
	 }

	 switch(parity){
		 default:
		 case 'n':
		 case 'N':
			 options.c_cflag &= ~PARENB;   //disable output & input parity
			 options.c_iflag &= ~INPCK; 	//Disable input parity checking
			 break;
		 
		 case 'o':
		 case 'O':
			 options.c_cflag |= (PARODD | PARENB);	//enable output & input parity , set odd parity 
			 options.c_iflag |= INPCK;			   //enable input parity checking
			 break;

		 case 'e':
		 case 'E':
			 options.c_cflag |= PARENB; 	//enable output & input parity
			 options.c_cflag &= ~PARODD;   //set even parity
			 options.c_iflag |= INPCK;		 //enable input parity checking
			 break;

		 case 'S':
		 case 's':	//as no parity
			 options.c_cflag &= ~PARENB;
			 options.c_cflag &= ~CSTOPB;
			 options.c_iflag &= ~INPCK;
			 break;
	 }
	 
	 options.c_cc[VTIME] = 1;
	 options.c_cc[VMIN] = 0;
	 
	 //set stop bit
	 switch(stopbits){
		 case 1:
			 options.c_cflag &= ~CSTOPB;
			 break;

		 case 2:
			 options.c_cflag |= CSTOPB;
			 break;

		 default:
			fprintf(stderr, "Unsupported stop bits\n");
			 return -1;
	 }
	 
	 tcflush(fd,TCIFLUSH); /* update the options */
	 
	 if(tcsetattr(fd,TCSANOW,&options) != 0)
		fprintf(stderr, "uart_setParity: tcsetattr failed\n");
	 
	 return 0;
 }


/*uart_init()
 *@brief	 init uart with baudrate
 *@param	 dev: device path
 *@param	 arg: argument to init uart
 *@return	 fd: uart file descriptor
 */
int uart_init(const char *dev, int baud_rate, uint8_t databits,
				uint8_t parity, uint8_t stopbits)
{
	 int fd;
	 
	 //open uart
	 fd = open( dev, O_RDWR | O_NOCTTY | O_NONBLOCK);
	 if(fd == -1)
		fprintf(stderr, "uart_init: failed to open uart port.\n");
	 
	 //set baud rate
	 uart_set_speed(fd,baud_rate);
	 
	 //set parity
	 if(uart_set_parity(fd,databits,stopbits,parity) == -1)
		fprintf(stderr, "uart_setParity: failed to set parity.\n");
	 
	 //set FNDELAY
	 fcntl(fd,F_SETFL,FNDELAY);
	 
	 return fd;
}

