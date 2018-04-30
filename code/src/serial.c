/* ------------------------------------------------------------ *
 * file:        serial.c                                        *
 * purpose:     Read ve.direct serial line data from victron    *
 *              BlueSolar charge controllers and store the data *
 *              block in a struct for further processing.       *
 *              the main functions are:                         *
 *                 config_serial()                              *
 *		   get_serial()                                 *
 *              Those are called from getvictron.c.             *
 *                                                              *
 * author:      03/30/2018 Frank4DD http://github.com/fm4dd     *
 *                                                              *
 * Open STREAMS device and receive data                         *
 *                                                              *
 * ve.direct transmits by default in text mode, a data block    *
 * is send continously at 1s intervals. Data blocks contain     *
 * several output lines, their format is as follows:            *
 * 0x0d 0x0a      carriage return + newline, 2 bytes            *
 * <fieldlabel>   string, min 1, max 8 chars                    *
 * 0x09           horizontal tab separator                      *
 * <field value>  string, min 1, max undef, define as 32 bytes  *
 *                                                              *
 * Last data block has field label "Checksum". Field value is   *
 * one byte calculated as modulo 256,which is the sum of each   *
 * ascii chars transmitted in the data block.                   *
 *                                                              *
 * Note transmission starts with CRNL, therefore first line is  *
 * always an empty line, and Checksum line wont get terminated  *
 * until a new transmission starts 1 second later.              *
 * ------------------------------------------------------------ */
#include <stdio.h>
#include <stdlib.h>	
#include <string.h>
#include <unistd.h>                                                          
#include <fcntl.h>                                                                                                               
#include <termios.h>
#include <poll.h>	
#include <errno.h>

#define BAUDRATE B19200

/* ------------------------------------------------------------ *
 * function config_serial() sets the serial line parameters.    *
 * arguments: serial device file descriptor, speed, parity      *
 * return code: 0 = success, -1 for errors                      *
 * ------------------------------------------------------------ */
int config_serial(int fd, int speed, int parity) {
   struct termios tty;
   memset(&tty, 0, sizeof tty);

   if(tcgetattr (fd, &tty) != 0) {
      printf("Error: Received error %d from tcgetattr", errno);
      return(-1);
   }

   /* ------------------------------------------------------------ *
    * Configure serial line speed set under BAUDRATE               *
    * ------------------------------------------------------------ */
   cfsetospeed(&tty, speed);
   cfsetispeed(&tty, speed);

   /* ------------------------------------------------------------ *
    * Configure serial line control mode flags                     *
    * ------------------------------------------------------------ */
   tty.c_cflag = (tty.c_cflag & ~CSIZE);   // clear char size mask
   tty.c_cflag |= CS8|CREAD|CLOCAL;        // 8n1, see termios.h for more information
   tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
   tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl
   tty.c_cflag |= parity;
   tty.c_cflag &= ~CSTOPB;
   tty.c_cflag &= ~CRTSCTS;
   /* ------------------------------------------------------------ *
    * Set blocking mode                                            *
    * ------------------------------------------------------------ */
   tty.c_cc[VMIN]  = 1;                 // 1 blocking, 0 nonblock
   tty.c_cc[VTIME] = 20;                // 20 = 20sec read timeout

   /* ------------------------------------------------------------ *
    * Configure serial line input mode flags                       *
    * ------------------------------------------------------------ */
   tty.c_iflag &= ~(IGNBRK | BRKINT);     // convert break to null byte
   tty.c_iflag &= ~(ICRNL | INLCR);       // CR to NL translation, no NL to CR translation
   tty.c_iflag &= ~(PARMRK | INPCK);      // don't mark parity errors or breaks, no input parity check
   tty.c_iflag &= ~(ISTRIP);              // don't strip high bit off, 
   tty.c_iflag &= ~(IXON | IXOFF | IXANY);// shut off xon/xoff SW flow control

   /* ------------------------------------------------------------ *
    * Configure serial line local mode flags                       *
    * ------------------------------------------------------------ */
   tty.c_lflag = 0;                       // no signaling chars, no echo, no canonical processing

   /* ------------------------------------------------------------ *
    * Configure serial output flags (no data out so we set to 0)   *
    * ------------------------------------------------------------ */
   tty.c_oflag = 0;

   /* ------------------------------------------------------------ *
    * Set the serial line attributes                               *
    * ------------------------------------------------------------ */
   if(tcsetattr (fd, TCSANOW, &tty) != 0) {
      printf("Error: Received error %d from tcsetattr", errno);
      return(-1);
   }
   return(0);
}

int poll_serial(int fd, char * serbuf) {
   struct pollfd fds[1];
   fds[0].fd = fd;
   fds[0].events = POLLRDNORM;
   int ret = 0;
   int readbytes = 0;

   /* ------------------------------------------------------------ *
    * poll() waits for data on serial line until 2.5s timeout.  *
    * ------------------------------------------------------------ */
   ret = poll(fds, 1, 2000);
   if (ret > 0) {                       // no timeout, data received
      if(fds[0].revents & POLLHUP) printf("Hangup\n");
      if(fds[0].revents & POLLRDNORM) {
         readbytes = read(fd, serbuf, 512);
         serbuf[readbytes]= '\0';             // terminate buffer
      }
   }
   return readbytes;
}

/* ------------------------------------------------------------ *
 * function get_serial() opens the serial line and polls data.  *
 * arguments: serial device file descriptor, ptr to data block  *
 * return code: 0 = success, -1 for errors                      *
 * ------------------------------------------------------------ */
int get_serial(char *device, char *serbuf, int verbose) { 
   /* ------------------------------------------------------------ *
    * Open serial device                                           *
    * ------------------------------------------------------------ */
   int fd;                                                             
   fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);

   if(fd == 0) {
      perror(device);
      printf("Error: Failed to open %s\n", device);
      return(-1); 
   }

   if(! isatty(fd)) {
      printf("Error: %s is not a TTY device\n", device);
      return(-1); 
   }
 
   /* ------------------------------------------------------------ *
    * Configure serial, define 19200 Baud, 8N1, no flow control.   *
    * ------------------------------------------------------------ */
   config_serial(fd, BAUDRATE, 0);      // set 19200 bps, 8n1 (no parity)

   /* ------------------------------------------------------------ *
    * Poll serial line data for 2 seconds                          *
    * ------------------------------------------------------------ */
   int bytes = poll_serial(fd, serbuf);
   if(verbose == 1) printf("Debug: received serial line data [%d] bytes\n", bytes);

   if(bytes < 100) return(-1);
   else return(0);
}
