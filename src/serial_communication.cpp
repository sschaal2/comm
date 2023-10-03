/*!=============================================================================
  ==============================================================================

  \file    serial_communication.cpp

  \author  Stefan Schaal
  \date    Oct 2011, upgraded to CPP in Aug. 2020

  ==============================================================================
  \remarks

  generic routines for managing serial communiction

  ============================================================================*/


#include <iostream>
#include <cstdlib>
#include "termios.h"
#include "fcntl.h"
#include "sys/ioctl.h"
#include "unistd.h"

#include "serial_communication.h"

// local variables 

// global variables 

// local functions

namespace serial_communication {

/*!*****************************************************************************
 *******************************************************************************
 \note  SerialCommuniction
 \date  Jan 2016

 \remarks

 Opens a serial communiction with geeven arguments


 *******************************************************************************
 Function Parameters: [in]=input,[out]=output

 \param[in]     fname : name of serial port
 \param[in]     baud  : baudrate (choose from termios.h, e.g., B38400)
 \param[in]     mode  : O_RDONLY or O_RDWR

 ******************************************************************************/
SerialCommunication::
SerialCommunication(char *fname, int baud, int mode)
{

  int serial_fd;
  struct termios options;

  // the serial port is not active until properly initialized
  active_ = false;
  fd_ = (int) NULL;

  serial_fd = open( fname, mode  | O_NOCTTY | O_NDELAY );

  if (serial_fd == -1) {
    printf("Can't open serial port %s for mode %d\n",fname,mode);
    return;
  }

  // get settings of the serial port
  tcgetattr(serial_fd, &options);

  // set baud rate
  cfsetispeed(&options, baud);
  cfsetospeed(&options, baud);

  // PARITY: NONE, 1 Stopbit, 8bits bytesize, disable hardware flow control
  options.c_cflag &= ~PARENB; 
  options.c_cflag &= ~CSTOPB;
  options.c_cflag |= CS8;
  options.c_cflag &= ~CRTSCTS;

  // make this a raw communication
  cfmakeraw(&options);

  // and update the serial line
  tcsetattr(serial_fd, TCSANOW, &options);

  // clear the buffer
  clearSerial();

  active_ = true;
  fd_ = serial_fd;

  printf("Serial port %s for mode %d opened successfully\n",fname,mode);  
  
}

/*!*****************************************************************************
 *******************************************************************************
 \note  ~SerialCommuniction
 \date  Jan 2016

 \remarks

 Closes the serial port


 *******************************************************************************
 Function Parameters: [in]=input,[out]=output

 none

 ******************************************************************************/
SerialCommunication::
~SerialCommunication()
{

  if (active_)
    close(fd_);
}

/*!*****************************************************************************
 *******************************************************************************
\note  clearSerial
\date  Oct 2000
   
\remarks 

empties the serial buffer

*******************************************************************************
Function Parameters: [in]=input,[out]=output

none

******************************************************************************/
int SerialCommunication::
clearSerial()
{
  if (!active_)
    return false;
  
  if (!tcflush(fd_,TCIOFLUSH))
    return false;

  return true;
}


/*!*****************************************************************************
 *******************************************************************************
\note  readSerial
\date  Oct 2000
   
\remarks 

reads into a buffer

*******************************************************************************
Function Parameters: [in]=input,[out]=output

\param[in]     n_bytes: number of bytes to read
\param[out]    buffer: buffer for read

returns the number of bytes actually read

******************************************************************************/
int SerialCommunication::
readSerial(int n_bytes, char *buffer) 
{
  if (!active_)
    return false;
  
  return read(fd_, buffer, (size_t) n_bytes);
}

/*!*****************************************************************************
 *******************************************************************************
\note  writeSerial
\date  Oct 2000
   
\remarks 

write to the serial port

*******************************************************************************
Function Parameters: [in]=input,[out]=output

\param[in]     n_bytes: number of bytes to write
\param[in]     buffer: buffer with bytes to write

returns the number of bytes actually written

******************************************************************************/
int SerialCommunication::
writeSerial(int n_bytes, char *buffer) 
{
  if (!active_)
    return false;

  return write(fd_, buffer, (size_t) n_bytes);
}

/*!*****************************************************************************
 *******************************************************************************
\note  checkSerial
\date  Oct 2000
   
\remarks 

        checks the number of bytes in serial input buffer

 *******************************************************************************
 Function Parameters: [in]=input,[out]=output

 \param[in]     fd : file descriptor

     returns the number of bytes in buffer

 ******************************************************************************/
int SerialCommunication::
checkSerial() 
{
  int n_bytes;

  if (!active_)
    return false;

  ioctl(fd_,FIONREAD,&n_bytes);

  return n_bytes;
}

}
